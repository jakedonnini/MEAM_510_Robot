const char nip[] PROGMEM = R"===( 
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Motor Control</title>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <style>
      #joystick-container { width: 400px; height: 400px; margin: 100px auto; background-color: #f0f0f0; border-radius: 50%; position: relative; }
      #joystick { width: 60px; height: 60px; background-color: red; border-radius: 50%; position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); }
    </style>
</head>
<body>
    <h1>ESP32 Motor Control Group 5</h1>
    <div id='joystick-container'>
        <div id='joystick'></div>
    </div>
<script>
  var container = document.getElementById('joystick-container');
  var joystick = document.getElementById('joystick');
  var containerRect = container.getBoundingClientRect();
  var isMoving = false;
  var maxDistance = containerRect.width / 2 - 30;
  var distance_all = 0;
  var angle_all = 0;

  function moveJoystick(event) {
    if (!isMoving) return;
    var x = event.clientX || event.touches[0].clientX;
    var y = event.clientY || event.touches[0].clientY;
    var dx = x - containerRect.left - containerRect.width / 2;
    var dy = y - containerRect.top - containerRect.height / 2;
    var distance = Math.sqrt(dx * dx + dy * dy);
    var angle = Math.atan2(dy, dx) * (180 / Math.PI);
    angle = angle < 0 ? angle + 360 : angle;

    if (distance > maxDistance) {
      var ratio = maxDistance / distance;
      dx *= ratio;
      dy *= ratio;
      distance = maxDistance;
    }

    joystick.style.transform = 'translate(' + (dx - 30) + 'px, ' + (dy - 30) + 'px)';
   	distance_all = Math.round(distance);
    angle_all = Math.round(angle);
  }

  container.addEventListener('mousedown', function() { isMoving = true; });
  container.addEventListener('touchstart', function() { isMoving = true; });
  document.addEventListener('mouseup', function() { isMoving = false; joystick.style.transform = 'translate(-50%, -50%)';distance_all = 0;angle_all = 0;update(); });
  document.addEventListener('touchend', function() { isMoving = false; joystick.style.transform = 'translate(-50%, -50%)';distance_all = 0;angle_all = 0;update(); });
  document.addEventListener('mousemove', moveJoystick);
  document.addEventListener('touchmove', moveJoystick);
  
  function updateMotorSpeed(speed) {
  	let xhttp = new XMLHttpRequest();
    
    let str = "speed?val=";
    let res = str.concat(speed);
    xhttp.open("GET", res, true);
    xhttp.send();
  }
        
  function updateMotorDirection(direction) {
  	let xhttp = new XMLHttpRequest();
    
    let str = "dir?val=";
    let res = str.concat(direction);
    xhttp.open("GET", res, true);
    xhttp.send();
  }
  
  function periodicFunction() {
    if (angle_all == 0) return;
      updateMotorSpeed(distance_all);
      updateMotorDirection(angle_all);
  }

  function update() {
      updateMotorSpeed(0);
      updateMotorDirection(0);
  }

const intervalId = setInterval(periodicFunction, 250);

</script>

</body>
</html>
)===";