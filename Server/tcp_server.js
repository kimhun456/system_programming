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

		console.log("recieved data : " + received_data);

		var datas = received_data.split("/");

		console.log(datas);


		if(datas[1]=="lock" && datas[2] =="0"){
				broadcast("android/camera");
		}
		if(datas[1] == "lock" && datas[2]=="1" ){
				broadcast("android/stop");
		}


		broadcast(data);
		console.log("send data  : " + received_data);
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
	});
}

//https://gist.github.com/creationix/707146
