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

const client = mqtt.connect(mqttBroker);

// Cache the last received moisture message
let lastMoisture = null;

// Connect to MQTT broker
client.on('connect', () => {
  console.log('Connected to MQTT broker');
  client.subscribe(topicMoisture, () => console.log('Subscribed to moisture topic'));
});

// Receive MQTT messages and broadcast via Socket.IO
client.on('message', (topic, message) => {
  lastMoisture = message.toString();
  io.emit('update', lastMoisture);
});

// Serve HTML from "public" folder
app.use(express.static('public'));

// Handle Socket.IO connections
io.on('connection', socket => {
  console.log('Client connected');

  // Send last cached moisture values immediately
  if (lastMoisture) {
    socket.emit('update', lastMoisture);
  }

  // Receive pump commands from clients
  socket.on('pumpCommand', data => {
    // data: {zone:1, action:"on"/"off"}
    console.log('Pump command received:', data);
    client.publish(topicCommand, JSON.stringify(data));
  });
});

server.listen(3000, () => console.log('Server running on http://localhost:3000'));
