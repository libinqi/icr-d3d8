#include "DCRead.h"
#include <iostream>
#include <string>
#include <stdio.h>
#include <windows.h>

using namespace v8;

Persistent<Function> DCRead::constructor;

struct DelayBaton
{
    // required
    uv_work_t request;                       // libuv
    uv_async_t async_request;                // libuv
    Persistent<Function> cbRealDataCallback; // javascript read callback
    Persistent<Function> cbErrorCallback;    // javascript error callback

    int isRead = 1;   // 是否开始监听
    int cardType = 1; // 卡类型：身份证：1，IC卡：2
    int status = 1;   // 状态1:正常，0：读取失败，-1：读取异常

    char *code = new char[19];       //身份证号码
    DWORD interval = 0;
    unsigned long cardsnr;
};

DelayBaton *baton;

HANDLE icdev; //设备句柄

DCRead::DCRead()
{
    icdev = 0;
}

DCRead::~DCRead()
{
}

static std::string GBKToUTF8(const std::string &strGBK)
{
    static std::string strOutUTF8 = "";
    WCHAR *str1;
    int n = MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, NULL, 0);
    str1 = new WCHAR[n];
    MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, str1, n);
    n = WideCharToMultiByte(CP_UTF8, 0, str1, -1, NULL, 0, NULL, NULL);
    char *str2 = new char[n];
    WideCharToMultiByte(CP_UTF8, 0, str1, -1, str2, n, NULL, NULL);
    strOutUTF8 = str2;
    delete[] str1;
    str1 = NULL;
    delete[] str2;
    str2 = NULL;
    return strOutUTF8;
}

void DCRead::Init(Handle<Object> exports)
{
    Isolate *isolate = Isolate::GetCurrent();

    // Prepare constructor template
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "DCRead"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Prototype
    NODE_SET_PROTOTYPE_METHOD(tpl, "StartRealReadCard", DC_Start_RealReadCard);
    NODE_SET_PROTOTYPE_METHOD(tpl, "StopRealReadCard", DC_Stop_RealReadCard);

    constructor.Reset(isolate, tpl->GetFunction());
    exports->Set(String::NewFromUtf8(isolate, "DCRead"),
                 tpl->GetFunction());
}

void DCRead::New(const FunctionCallbackInfo<Value> &args)
{
    Isolate *isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    if (args.IsConstructCall())
    {
        // Invoked as constructor: `new MyObject(...)`

        DCRead *obj = new DCRead();
        obj->Wrap(args.This());
        args.GetReturnValue().Set(args.This());
    }
    else
    {
        // Invoked as plain function `MyObject(...)`, turn into construct call.
        const int argc = 1;
        Local<Value> argv[argc] = {args[0]};
        Local<Function> cons = Local<Function>::New(isolate, constructor);
        args.GetReturnValue().Set(cons->NewInstance(argc, argv));
    }
}

// libuv异步线程函数
void Delay(uv_work_t *req)
{
    DelayBaton *baton = static_cast<DelayBaton *>(req->data);
    try
    {
        do
        {
            unsigned long cardsnr;
            int result = dc_card(icdev, 0, &cardsnr);
            if (result == 0)
            {
                if (((GetTickCount() - baton->interval) > 500) ||
                    (baton->cardsnr != cardsnr))
                {
                    baton->cardType = 2;
                    sprintf(baton->code, "%2X", cardsnr);
                    baton->status = 1;
                    baton->async_request.data = baton;
                    uv_async_send(&baton->async_request);
                }

                baton->interval = GetTickCount();
                baton->cardsnr = cardsnr;
            }
        } while (baton->isRead);
    }
    catch (Exception)
    {
        baton->status = -1;
        baton->async_request.data = baton;
        uv_async_send(&baton->async_request);
    }
}

// libuv异步线程完成函数
void DelayAfter(uv_work_t *req, int status)
{
    DelayBaton *baton = static_cast<DelayBaton *>(req->data);
    uv_close((uv_handle_t *)&baton->async_request, NULL);
    baton->cbRealDataCallback.Reset();
}

// libuv异步事件循环完成回调函数
void DelayAsyncAfter(uv_async_t *req, int status)
{
    Isolate *isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    // 获取引用上下文
    DelayBaton *baton = static_cast<DelayBaton *>(req->data);

    if (baton->status)
    {
        Local<Object> readInfo = Object::New(isolate);
        readInfo->Set(String::NewFromUtf8(isolate, "cardType"), Number::New(isolate, baton->cardType));

        std::string cardIdStr = baton->code;
        cardIdStr = cardIdStr.substr(6, 2) + cardIdStr.substr(4, 2) + cardIdStr.substr(2, 2) + cardIdStr.substr(0, 2);
        // int cardId = strtol(cardIdStr.c_str(), NULL, 16);
        readInfo->Set(String::NewFromUtf8(isolate, "code"), String::NewFromUtf8(isolate, cardIdStr.c_str()));

        // if (baton->cardType == 1)
        // {
        //     readInfo->Set(String::NewFromUtf8(isolate, "name"), String::NewFromUtf8(isolate, GBKToUTF8(baton->name).c_str()));
        //     readInfo->Set(String::NewFromUtf8(isolate, "gender"), String::NewFromUtf8(isolate, GBKToUTF8(baton->gender).c_str()));
        //     readInfo->Set(String::NewFromUtf8(isolate, "folk"), String::NewFromUtf8(isolate, GBKToUTF8(baton->folk).c_str()));
        //     readInfo->Set(String::NewFromUtf8(isolate, "birthDay"), String::NewFromUtf8(isolate, baton->birthDay));
        //     readInfo->Set(String::NewFromUtf8(isolate, "code"), String::NewFromUtf8(isolate, baton->code));
        //     readInfo->Set(String::NewFromUtf8(isolate, "address"), String::NewFromUtf8(isolate, GBKToUTF8(baton->address).c_str()));
        //     readInfo->Set(String::NewFromUtf8(isolate, "agency"), String::NewFromUtf8(isolate, GBKToUTF8(baton->agency).c_str()));
        //     readInfo->Set(String::NewFromUtf8(isolate, "expireStart"), String::NewFromUtf8(isolate, baton->expireStart));
        //     readInfo->Set(String::NewFromUtf8(isolate, "expireEnd"), String::NewFromUtf8(isolate, baton->expireEnd));
        // }
        // else
        // {
        //     std::string cardIdStr = baton->code;
        //     cardIdStr = cardIdStr.substr(6, 2) + cardIdStr.substr(4, 2) + cardIdStr.substr(2, 2) + cardIdStr.substr(0, 2);
        //     int cardId = strtol(cardIdStr.c_str(), NULL, 16);
        //     readInfo->Set(String::NewFromUtf8(isolate, "code"), Number::New(isolate, cardId));
        // }

        // 获取回调函数
        Local<Function> cb = Local<Function>::New(isolate, baton->cbRealDataCallback);
        const unsigned argc = 1;
        Local<Value> argv[argc] = {
            readInfo};
        cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);
    }
    else if (baton->status == 0)
    {
        Local<Function> cb = Local<Function>::New(isolate, baton->cbErrorCallback);
        const unsigned argc = 1;
        Local<Value> argv[argc] = {String::NewFromUtf8(isolate, "Read the card information failure!")};
        cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);
    }
    else
    {
        Local<Function> cb = Local<Function>::New(isolate, baton->cbErrorCallback);
        const unsigned argc = 1;
        Local<Value> argv[argc] = {String::NewFromUtf8(isolate, "Read the card information exception!")};
        cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);
    }
}

void DCRead::DC_Start_RealReadCard(const FunctionCallbackInfo<Value> &args)
{
    Isolate *isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    int iPort = args[0]->Int32Value();
    Local<Function> cb = Local<Function>::Cast(args[2]);

    icdev = dc_init(iPort, 115200);
    if (icdev)
    {
        baton = new DelayBaton;
        baton->request.data = baton;
        baton->cbRealDataCallback.Reset(isolate, Persistent<Function>::Persistent(isolate, Local<Function>::Cast(args[1])));
        baton->cbErrorCallback.Reset(isolate, cb);

        // 事件循环队列异步函数
        // node.js默认事件循环机制
        uv_loop_t *loop = uv_default_loop();
        uv_async_init(loop, &baton->async_request, (uv_async_cb)DelayAsyncAfter);
        uv_queue_work(loop, &baton->request, Delay, DelayAfter);
        uv_run(loop, UV_RUN_NOWAIT);
    }
    args.GetReturnValue().Set(Number::New(isolate, (int)icdev));
}

void DCRead::DC_Stop_RealReadCard(const FunctionCallbackInfo<Value> &args)
{
    Isolate *isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);

    int result = dc_exit(icdev);
    baton->isRead = 0;
    args.GetReturnValue().Set(Number::New(isolate, result));
}

void InitAll(Handle<Object> exports)
{
    DCRead::Init(exports);
}

NODE_MODULE(DCRead, InitAll)
