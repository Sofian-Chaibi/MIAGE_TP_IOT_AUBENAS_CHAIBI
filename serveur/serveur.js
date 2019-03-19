var http = require('http');
var fs = require('fs');
// Chargement de socket.io
var WebSocketServer = require('websocket').server;

// mqtt listener
var mqtt = require('mqtt')
var client  = mqtt.connect('mqtt://10.154.125.62') // mettre votre adresse IP  "mqtt://xxx.xxx.xxx.xxx"


var http = require('http');
var fs = require('fs');
var path = require("path");
var url = require("url");

// Gestion des pages HTML
function sendError(errCode, errString, response) {
  response.writeHead(errCode, {"Content-Type": "text/plain"});
  response.write(errString + "\n");
  response.end();
  return;
}

function sendFile(err, file, response) {
  if(err) return sendError(500, err, response);
  response.writeHead(200);
  response.write(file, "binary");
  response.end();
}

function getFile(exists, response, localpath) {
  if(!exists) return sendError(404, '404 Not Found', response);
  fs.readFile(localpath, "binary",
   function(err, file){ sendFile(err, file, response);});
}

function getFilename(request, response) {
  var urlpath = url.parse(request.url).pathname; 
  var localpath = path.join(process.cwd(), urlpath); 
  fs.exists(localpath, function(result) { getFile(result, response, localpath)});
}
// Chargement du fichier index.html affiché au client
var server = http.createServer(getFilename);

client.on('connect', function () {
  console.log('connection');
  client.subscribe('miage1/menez/sensors/temp', error());
  client.subscribe('miage1/menez/sensors/light',error());
  client.subscribe('miage1/menez/sensors/error',error());
})


client.on('message', function (topic, message) {
  // message is Buffer
  var prelude="";
  if(topic==="miage1/menez/sensors/temp")
  {
	  prelude= "Temp : "
  }
  if(topic==="miage1/menez/sensors/light")
  {
	  prelude= "Light : "
  }
  console.log(prelude+message.toString())
  if(connections[1] !== undefined)
  {
  connections[1].send("salle 1 - "+prelude+message.toString());
  }
})

wsServer = new WebSocketServer({
    httpServer: server
});
console.log("Serveur crée");
var connections = [];
// WebSocket server
wsServer.on('request', function (request) {
    var connection = request.accept(null, request.origin);
	console.log("connection Reussi");
    // This is the most important callback for us, we'll handle
    // all messages from users here.
    connection.on('message', function (message, connection=this) {
            if (message.type === 'utf8') {
				console.log(message);
                // process WebSocket message
                if (message.utf8Data === "Arduino") {
					console.log("Arduino");
					console.log("Socket Sauvegardé");
                    connections[0] = connection;
                }
                if (message.utf8Data === 'Web') {
					console.log("Site Web");
					console.log("Socket Sauvegardé");
                    connections[1] = connection;
                }
                
            }

            if (connection === connections[0] && connections[1] !== undefined) {
				console.log("J'envois à web");
                connections[1].send("salle 1 - "+message.utf8Data);

            }
            if (connection === connections[1] && connections[0] !== undefined) {
				console.log("J'envois à la page esp32");
				if(message.utf8Data.toString().includes("MQTT -"))
				{	var subString = message.utf8Data.toString().substring(7,8);
					if(subString.toString().includes("1"))
					{
						client.publish('miage1/menez/sensors/led',"ON");
					}
					if(subString.includes("0"))
					{
						client.publish('miage1/menez/sensors/led',"OFF");
					}
				
				}
				
                connections[0].send(message.utf8Data);
            }
        }
    );


});

function error (err){
    if (err) {
      console.log("oups problème de connexion broker");
    }
	else{
		console.log("en place !");
	}
}




server.listen(8080);
