var addon = require('../bin/DCRead.node');
var dcRead = new addon.DCRead();

exports.startRealReadCard = function (port, realCallback, errorCallback) {
    port = port || 100;
    var result = dcRead.StartRealReadCard(port, realCallback, errorCallback);
    return result;
};

exports.stopRealReadCard = function () {
    var result = dcRead.StopRealReadCard();
    return result;
};

