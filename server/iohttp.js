var express = require('express');
var app = express();

var http = require('http').Server(app);
var bodyParser = require('body-parser');
var child_process = require('child_process');
var spawn = require('child_process').spawn;


app.use(bodyParser.urlencoded({ extended: false }))
app.use(bodyParser.json());
app.use(express.static(__dirname + '/app'));
app.all('*', function (req, res, next) {
    res.header("Access-Control-Allow-Origin", "*");
    res.header("Access-Control-Allow-Headers", "X-Requested-With");
    res.header("Access-Control-Allow-Methods", "PUT,POST,GET,DELETE,OPTIONS");
    res.header("X-Powered-By", ' 3.2.1');
    //res.header("Content-Type", "application/json;charset=utf-8");
    next();
});

//var pd = require('./protocol.js');
http.listen(8080, function () {
    console.log('listening on *:8080');
});


function execute_server() {
    const ls = spawn('server.exe', ["test"]);
    ls.stdout.setEncoding('utf8');
    ls.stdout.on('data', function (data) {
        console.log(data);
    });

    ls.stderr.on('data', function (data) {
        //console.log(`stderr: ${data}`);
        console.log('error occur:' + data);
    });

    ls.on('close', function (code) {
        //console.log(`child process exited with code ${code}`);
        console.log("error out");
    });

    //console.log("ready to send");
    function send_2child(type, data) {
        var cam = { type: type, data: data };
        var str = JSON.stringify(cam);
        str += "\n";
        ls.stdin.write(str);
    }

}

app.get('/', function (req, res) {
    res.send('<h1>Welcome Realtime TCP server of master node </h1>');
});


//启动一个端口进程
app.get('/port_start/:port', function (req, res) {

});
//重启一个端口进程
app.get('/port_restart/:port', function (req, res) {

});
//停止一个端口进程
app.get('/port_stop/:port', function (req, res) {

});
//获取进程端口列表
app.get('/port_list', function (req, res) {

});
//向端口设备发布命令
app.post('/port_command/:port/:deviceid', function (req, res) {

});
app.post('/data/:port/:deviceid', function (req, res) {
    console.log("body is ", req.body);
    res.send(req.body);
    // res.send("receive ok\r\n");
});
app.post('/protocol/VIP', function (req, res) {

});
app.post("/test",function(req,res){
	console.log(req.body);
	res.send({ret:1,info:"it is a test"});
});