try {
	var dcRead = require('./lib/DCRead');
	var result = dcRead.startRealReadCard(100, function (cardInfo) {
		console.log('卡类型:', cardInfo.cardType);
		console.log('卡号:', cardInfo.code);
	}, function (err) {
		console.log('读取卡信息失败——', err);
	})
	console.log('初始化读卡设备：', result > 0 ? '成功' : '失败');
} catch (error) {
	console.info(error);
}

//setTimeout(function () {
//    iDR.StopRealReadCard();
//}, 20000)

process.on('uncaughtException', function (err) {
	console.log(err);
});

process.stdin.resume();