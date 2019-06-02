var url = require('url');
var http = require('http');



function post_data(obj, options, token, cb) {
    //console.log(obj);
    var content = JSON.stringify(obj);
    var jsonheaders = {
        'Content-Type': 'application/json',
        'Content-Length': content.length,
        //'token': token
    };
    options.headers = jsonheaders;
    var req = http.request(options, function (res) {
        res.setEncoding('utf8');
        if (res.statusCode == 200) {
            var body = "";
            res.on('data', function (data) {
                body += data;
            });
            res.on('end', function () {
                if (cb != null)
                    cb(200, body);
            });
        }
        else {
            if (cb != null)
                cb(500, -1);
        }
    });

    req.on('error', function (e) {
        console.log('problem with request: ' + e.message);
        if (cb != null) {
           // cb(-1,e.message);
        }
    });

    req.write(content);
    req.end();
}

//16字节包含crc和头部长度
//protocol 协议 callback  回调函数; 
function parseurl(urlpath) {
    //var a = url.parse('http://sxzd365.com:8080/one?a=index&t=article&m=default');
    //console.log(a);
    if (urlpath == null)
        return null;
    var ret = url.parse(urlpath);
    //var pro = ret.protocol;
    //var options = {
    //    hostname: host,//'127.0.0.1',
    //    port: port,// 80,
    //    path: routepath,//'/net/index.php?SerialPort/index/1000',
    //    method: 'POST',
    //    headers: jsonheaders
    //};

    var options = {};
    options.hostname = ret.hostname;
    options.port = ret.port;
    options.path = ret.path;
    options.method = "POST";
    // options.headers = jsonheaders;
    return options;
}

exports.parseurl = parseurl;
exports.post_data = post_data;
