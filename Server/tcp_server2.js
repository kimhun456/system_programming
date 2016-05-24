var net = require('net');
var port = 1337;
var clients = [];
var received_data;

var server = net.createServer(function(socket) {
	socket.setEncoding("utf8");
	socket.name = socket.remoteAddress + ":" + socket.remotePort ;
	// send(socket, "Echo Server");
	console.log("client name is : "+socket.name);
	clients.push(socket);
	console.log("----------"+socket.name + " is connected------");
	socket.on('data',function(data){
		received_data = data.toString();

		broadcast(data+"*");
		console.log("recieved data : " + received_data);

		var datas = received_data.split("/");

		console.log(datas);

		var len = datas.length;
		var sum="";
		for(var i=0;i<len-1;i++){
				sum+=datas[i]+"/";
		}
		console.log("convert data : "+sum);
//		broadcast(sum);
		if(datas[1]=="lock" && (datas[2] =="0"||data[2]=="0\n")){
				setTimeout(function(){
					broadcast("android/camera/");
				},1000);
				console.log("send data  : " + "android/camera/");
		}
		if(datas[1] == "lock" && (datas[2]=="1"||datas[2]=="1\n") ){
				setTimeout(function() {
					broadcast("android/stop/");
				}, 1000);
				console.log("send data  : " + "android/stop/");
		}


//		console.log("send data  : " + received_data);
	});

	socket.on("end", function(){
		console.log("--------client leave --------");
		clients.splice(clients.indexOf(socket),1);
	});

	socket.on("error",function(err){
		console.log(err);
	});

});


server.listen(port);

console.log("server start in 52.69.176.156 port 1337");

function broadcast(message){
	clients.forEach(function (client){
		client.write(message);
		console.log("send data : "+ message);
	});
}

//https://gist.github.com/creationix/707146
