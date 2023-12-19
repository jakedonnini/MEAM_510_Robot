const char web[] PROGMEM = R"===( 
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Driving Robot UI</title>
    <style>
        body, html {
            height: 100%;
            margin: 0;
            display: flex;
            justify-content: center;
            align-items: center;
        }

        .container {
            display: flex;
            align-items: flex-start;
        }

        .grid {
            position: relative;
            background-color: #f0f0f0;
            width: 600px; /* Width of the arena could be 1000 */
            height: 300px; /* Height of the arena was 500 */
            margin-right: 20px; /* Space between grid and control panel */
        }

        .robot, .trophy, .police-car, .fake-trophy {
            position: absolute;
            width: 20px;
            height: 20px;
        }

        .pos1, .pos2 {
            position: absolute;
            width: 5px;
            height: 5px;
            background-color: blue;
            border-radius: 50%;
        }

        .robot {
            background-color: red;
            border-radius: 50%;
        }

        .trophy {
            background-color: gold;
        }

        .police-car {
            background-color: lightblue;
            width: 70px;
            height: 40px;
        }

        .fake-trophy {
            background-color: silver;
        }

        .control-panel {
            width: 200px;
            padding: 10px;
            display: flex;
            flex-direction: column;
            align-items: center;
            background-color: #f4f4f4;
            border: 1px solid #333;
        }

        .search-button {
            width: flex;
            padding: 5px;
            display: flex;
            flex-direction: row;
            align-items: center;
            background-color: #f4f4f4;
            // border: 1px solid #333;
        }

        .button {
            width: 100%;
            margin-bottom: 10px;
            margin-right: 4px;
            padding: 10px;
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
        }

        #cordDisplay {
            padding: 10px;
            background-color: #fff;
            border: 1px solid #333;
            text-align: center;
        }

        #searchDisplay {
            padding: 10px;
            background-color: #fff;
            border: 1px solid #333;
            text-align: center;
        }

        #wallDisplay {
            padding: 10px;
            background-color: #fff;
            border: 1px solid #333;
            text-align: center;
        }

        #grabDisplay {
            padding: 10px;
            background-color: #fff;
            border: 1px solid #333;
            text-align: center;
        }

        #distDisplay {
            padding: 10px;
            background-color: #fff;
            border: 1px solid #333;
            text-align: center;
        }

        #polDisplay {
            padding: 10px;
            background-color: #fff;
            border: 1px solid #333;
            text-align: center;
        }

        .label {
            position: absolute;
            color: black;
            font-size: 8px;
            text-align: center;
            width: 20px;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
        }

        .arena-label {
            font-size: 14px;
            color: #333;
            margin-top: 5px;
        }
    </style>
</head>
<body>
    <div class="container">
        <div>
            <div id="gridContainer" class="grid" onclick="moveRobot(event)">
                <!-- Elements will be added here -->
            </div>
            <div class="arena-label">^ this corner (2000, 5000) </div>
        </div>
        <div class="control-panel">
            <div class="search-button">
                <button class="button" onclick="searchButtonStop()">Stop</button>
                <button class="button" onclick="searchButtonReal()">Real</button>
                <button class="button" onclick="searchButtonFake()">Fake</button>
            </div>
            <button class="button" onclick="wallTracing()">Wall Tracing</button>
            <button class="button" onclick="activateGrabber()">Grabber</button>
            <button class="button" onclick="police()">Police</button>
            <div id="cordDisplay">Coordinates: (0, 0)</div>
            <div id="searchDisplay">Trophy: Not Searching</div>
            <div id="wallDisplay">Wall Following: OFF</div>
            <div id="grabDisplay">Grabber: Open</div>
            <div id="distDisplay">0 0</div>
            <div id="polDisplay">Police: OFF</div>
        </div>
    </div>

    <script>
        const gridContainer = document.getElementById('gridContainer');
        const gridDimensions = { width: 600, height: 300 };
        const robotSize = { width: 20, height: 20 };
        var searchStat = "Not Searching";
        var grabStat = "Open";
        var wallStat = "OFF";
        var policeStat = "OFF";
        var policeSend = 0;
        var grabSend = 0;
        var wallSend = 0;
        var ideal_robotX = 0;
        var ideal_robotY = 0;
        var real_robotX1 = 0;
        var real_robotX2 = 0;
        var real_robotY1 = 0;
        var real_robotY2 = 0;
        var policeX = 0;
        var policeY = 0;
        var dist1 = 0;
        var dist2 = 0;

        function createGrid() {
            const robot = document.createElement('div');
            robot.classList.add('robot');
            placeElement(robot, 0, 0);
            createLabel(robot, 'Robot');

            const pos1 = document.createElement('div');
            pos1.classList.add('pos1');
            placeElement(pos1, real_robotX1, real_robotY1);

            const pos2 = document.createElement('div');
            pos2.classList.add('pos2');
            placeElement(pos2, real_robotX2, real_robotY2);

            const policeCar = document.createElement('div');
            policeCar.classList.add('police-car');
            placeElement(policeCar, policeX, policeY);
            createLabel(policeCar, 'Police');
        }

        function px_to_coordsX(x) {
            return Math.round(x * 400 + 2000);
        }

        function px_to_coordsY(y) {
            return Math.round(y * 400 + 3000);
        }

        function coords_to_pxX(x) {
            return (x-2000) / 400;
        }

        function coords_to_pxY(y) {
            return (y-3000) / 400;
        }

        function placeElement(element, x, y) {
            const adjustedX = Math.max(0, Math.min(gridDimensions.width - robotSize.width, (x / 10 * gridDimensions.width)));
            const adjustedY = Math.max(0, Math.min(gridDimensions.height - robotSize.height, (y / 5 * gridDimensions.height)));
            element.style.left = adjustedX + 'px';
            element.style.top = adjustedY + 'px';
            gridContainer.appendChild(element);
        }

        function createLabel(element, text) {
            const label = document.createElement('div');
            label.classList.add('label');
            label.textContent = text;
            element.appendChild(label);
        }

        function moveRobot(event) {
            const rect = gridContainer.getBoundingClientRect();
            ideal_robotX = Math.max(0, Math.min(10, ((event.clientX - rect.left) / rect.width) * 10));
            ideal_robotY = Math.max(0, Math.min(5, ((event.clientY - rect.top) / rect.height) * 5));
            updateRobotPosition(ideal_robotX, ideal_robotY);
            updateDistanceDisplay(ideal_robotX, ideal_robotY);

  	        let xhttp = new XMLHttpRequest();
            let str1 = "moveX?val=";
            let res1 = str1.concat(px_to_coordsX(ideal_robotX));
            console.log(res1);
            xhttp.open("GET", res1, true);
            xhttp.send();
            let xhttp1 = new XMLHttpRequest();
            let str2 = "moveY?val=";
            let res2 = str2.concat(px_to_coordsY(ideal_robotY));
            console.log(res2);
            xhttp1.open("GET", res2, true);
            xhttp1.send();
        }

        function updateRobotPosition(x, y) {
            const robot = document.querySelector('.robot');
            const adjustedX = Math.max(0, Math.min(gridDimensions.width - robotSize.width, (x / 10 * gridDimensions.width) - robotSize.width / 2));
            const adjustedY = Math.max(0, Math.min(gridDimensions.height - robotSize.height, (y / 5 * gridDimensions.height) - robotSize.height / 2));
            robot.style.left = adjustedX + 'px';
            robot.style.top = adjustedY + 'px';
        }

        function updateRobotRealPosition1(x, y) {
            const point = document.querySelector('.pos1');
            const adjustedX = Math.max(0, Math.min(gridDimensions.width - robotSize.width, (x / 10 * gridDimensions.width) - 5 / 2));
            const adjustedY = Math.max(0, Math.min(gridDimensions.height - robotSize.height, (y / 5 * gridDimensions.height) - 5 / 2));
            point.style.left = adjustedX + 'px';
            point.style.top = adjustedY + 'px';
        }

        function updateRobotRealPosition2(x, y) {
            const point = document.querySelector('.pos2');
            const adjustedX = Math.max(0, Math.min(gridDimensions.width - robotSize.width, (x / 10 * gridDimensions.width) - 5 / 2));
            const adjustedY = Math.max(0, Math.min(gridDimensions.height - robotSize.height, (y / 5 * gridDimensions.height) - 5 / 2));
            point.style.left = adjustedX + 'px';
            point.style.top = adjustedY + 'px';
        }

        function updatePolicePosition(x, y) {
            const point = document.querySelector('.police-car');
            const adjustedX = Math.max(0, Math.min(gridDimensions.width - robotSize.width, (x / 10 * gridDimensions.width) - 70 / 2));
            const adjustedY = Math.max(0, Math.min(gridDimensions.height - robotSize.height, (y / 5 * gridDimensions.height) - 40 / 2));
            point.style.left = adjustedX + 'px';
            point.style.top = adjustedY + 'px';
        }

        function police() {
            console.log("Police...");
  	        let xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
		        if (this.readyState == 4 && this.status == 200) {
                    policeStat = this.responseText;
		        }
	        };
            let str = "police?val=";
            // toggle
            if (policeSend == 0) {
                policeSend = 1;
            } else {
                policeSend = 0;
            }
            let res = str.concat(policeSend);
            xhttp.open("GET", res, true);
            xhttp.send();
            document.getElementById('polDisplay').textContent = `Police: ${searchStat}`;
        }

        function searchButtonReal() {
            console.log("Searching...");
  	        let xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
		        if (this.readyState == 4 && this.status == 200) {
                    searchStat = this.responseText;
		        }
	        };
            let str = "search?val=";
            let res = str.concat(2);
            xhttp.open("GET", res, true);
            xhttp.send();
            document.getElementById('searchDisplay').textContent = `Trophy: ${searchStat}`;
        }

        function searchButtonFake() {
            console.log("Searching...");
  	        let xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
		        if (this.readyState == 4 && this.status == 200) {
                    searchStat = this.responseText;
		        }
	        };
            let str = "search?val=";
            let res = str.concat(1);
            xhttp.open("GET", res, true);
            xhttp.send();
            document.getElementById('searchDisplay').textContent = `Trophy: ${searchStat}`;
        }

        function searchButtonStop() {
            console.log("Searching...");
  	        let xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
		        if (this.readyState == 4 && this.status == 200) {
                    searchStat = this.responseText;
		        }
	        };
            let str = "search?val=";
            let res = str.concat(0);
            xhttp.open("GET", res, true);
            xhttp.send();
            document.getElementById('searchDisplay').textContent = `Trophy: ${searchStat}`;
        }

        function wallTracing() {
            console.log("Wall tracing...");
  	        let xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
		        if (this.readyState == 4 && this.status == 200) {
                    wallStat = this.responseText;
		        }
	        };
            document.getElementById('wallDisplay').textContent = `Wall Following: ${wallStat}`;
            let str = "wall?val=";
             // toggle
            if (wallSend == 0) {
                wallSend = 1;
            } else {
                wallSend = 0;
            }
            let res = str.concat(wallSend);
            xhttp.open("GET", res, true);
            xhttp.send();
        }

        function activateGrabber() {
            console.log("Activating grabber...");
  	        let xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
		        if (this.readyState == 4 && this.status == 200) {
                    grabStat = this.responseText;
		        }
	        };
            document.getElementById('grabDisplay').textContent = `Grabber: ${grabStat}`;
            let str = "grab?val=";
             // toggle
            if (grabSend == 0) {
                grabSend = 1;
            } else {
                grabSend = 0;
            }
            let res = str.concat(grabSend);
            xhttp.open("GET", res, true);
            xhttp.send();
        }

        function updateDistanceDisplay(x, y) {
            document.getElementById('cordDisplay').textContent = `Coordinates: (${px_to_coordsX(x)}, ${px_to_coordsY(y)})`;
        }

        function getPosData() {
            let xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
		        if (this.readyState == 4 && this.status == 200) {
                    // "4000 5000 4100 5100 3000 3000"
                    // 4405 3519 4616 3473
                    // x1 y1 x2 y2 polx poly
                    const data = this.responseText;
                    const numbersArray = data.split(" ");

                    real_robotX1 = coords_to_pxX(parseInt(numbersArray[0]));
                    real_robotY1 = coords_to_pxY(parseInt(numbersArray[1]));
                    real_robotX2 = coords_to_pxX(parseInt(numbersArray[2]));
                    real_robotY2 = coords_to_pxY(parseInt(numbersArray[3]));
                    policeX = coords_to_pxX(parseInt(numbersArray[4]));
                    policeY = coords_to_pxY(parseInt(numbersArray[5]));
                    dist1 = parseInt(numbersArray[6]);
                    dist2 = parseInt(numbersArray[7]);
                    document.getElementById('distDisplay').textContent = `dist1: ${dist1} dist2: ${dist2}`;
		        }
	        };
            let str = "pos?val=";
            let res = str.concat(1);
            xhttp.open("GET", res, true);
            xhttp.send();
            updateRobotRealPosition1(real_robotX1, real_robotY1);
            updateRobotRealPosition2(real_robotX2, real_robotY2);
            updatePolicePosition(policeX, policeY);
        }

        const intervalId = setInterval(getPosData, 1000);

        createGrid();
    </script>
</body>
</html>

)===";