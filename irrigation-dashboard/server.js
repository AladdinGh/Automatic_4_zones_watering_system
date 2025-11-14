const express = require('express');
const http = require('http');
const socketIo = require('socket.io');
const mqtt = require('mqtt');

const app = express();
const server = http.createServer(app);
const io = socketIo(server);

const mqttBroker = 'mqtt://192.168.178.45';
const topicMoisture = 'irrigation/moisture';
const topicCommand = 'irrigation/command';
const topicBoardStatus = 'irrigation/boardstatus';

const client = mqtt.connect(mqttBroker);

let lastMoisture = null;
let lastBoardStatus = null;

client.on('connect', () => {
  console.log('Connected to MQTT broker');
  client.subscribe(topicMoisture);
  client.subscribe(topicBoardStatus);
});

// MQTT → Socket.IO
client.on('message', (topic, message) => {
  const data = message.toString();

  if (topic === topicMoisture) {
    lastMoisture = data;
    io.emit('update', JSON.parse(data));
  }

  if (topic === topicBoardStatus) {
    try {
      const parsed = JSON.parse(data);
      lastBoardStatus = parsed;
      io.emit('boardStatus', parsed);
    } catch (e) {
      console.error("Invalid JSON from ESP:", data);
    }
  }
});

app.use(express.static('public'));

io.on('connection', socket => {
  console.log("Client connected");

  if (lastMoisture) socket.emit('update', JSON.parse(lastMoisture));
  if (lastBoardStatus) socket.emit('boardStatus', lastBoardStatus);

  socket.on('pumpCommand', data => {
    console.log("Pump command:", data);
    client.publish(topicCommand, JSON.stringify(data));
  });
});

server.listen(3000, '0.0.0.0', () =>
  console.log("Server running on port 3000")
);
