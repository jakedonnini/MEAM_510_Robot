# MEAM_510_Robot Final Project
 ```
    * Team Number: 3
    * Robot Name: Black Beatle
    * Team Members: Jacob Donnini, Abdinajib Mohamed, Nhat Le
    * Description of hardware: Microcontrollers: 2 ESP32- C3, 1 ESP32- S2, 1 servos motor, 
      2 motors + encoders, 1 inverter, 1 H-bridge, 1 Level shifter, 3 TOF sensors, 1 voltage regulators,
      2 IR phototransistors, 2 Vive trackers photodiodes

```
![PXL_20231219_214855812](https://github.com/jakedonnini/MEAM_510_Robot/assets/24221155/5a0785ca-84ef-4776-b3e6-27d95a9d1de8)

# Project Requirements

Each performance task will run in 2 minutes. They may be reset and run sequentially. The robot may not be reprogrammed or changed between tasks, but sensors or web interface or on/off switches may be used to indicate which task to do. Robots and objects start in their game start positions at the beginning of each task.
Minimum requirements for passing Final Project.
Control of mobile base to reliably reach any of the 5 on-field objects.
Robot must transmit a ESP-NOW packet every time it receives communication.
Minimum requirements for full marks on Graded Evaluation. This will be tested as if during normal game play but with no other robots on the field. Robots will be tested to see if they can achieve the following within the normal game time:
Perform wall-following autonomously.
Use the Vive system to transmit your X-Y location via UDP broadcast, once per second.
Autonomously identify, go to and push either trophy or fake (using 550Hz  for trophy or 23Hz for fake beacon tracking) that is randomly placed.
Autonomously move to the police car box which is randomly placed and push it so the center of the car moves at least 10 inches.
 
Performance score. Do enough to get 50 pts for full marks. 65 pts are possible giving the possibility of 15pt extra credit.

- Control Robot 				(15pt controlled to reach object)
- Robot comms via ESP-NOW	 	(2pt sending with each message)
- Vive XY via UDP			(3pt sending at 1Hz with proper protocol)
- Follow wall – full circuit		(15pt partial credit: % of full circuit)
- Autonomous navigate to trophy	 (15pt partial credit: % distance to trophy (or fake) )
- Autonomous move police 10"		(15pt partial credit: 8pt reaching box, +1pt for each inch pushed from original spot.)


##### Black Beatle was able to get evertying besides Robot comms via ESP-Now resulting in 63/50 final score.Having ESP-Now and wifi proved a bit challenging which resulted in having one of them working by themselves but not together or we would get weird data when doing both.


Black Beatle, the robot designed for the MEAM 510 project, featured a comprehensive set of functionalities for achieving its objectives. It was equipped with a PI controller and wheel encoders for precise movement control, allowing it to navigate accurately to specified locations on the field, with an interactive website providing real-time location data. Communication was handled via ESP-NOW, ensuring consistent data transfer despite initial Wi-Fi challenges. The robot autonomously followed walls using Time-of-Flight (ToF) sensors for maintaining safe distances. Two Vive trackers provided accurate positioning information, crucial for navigation and strategy. For trophy identification, Black Beatle employed a servo mechanism with phototransistors, enabling it to detect and retrieve trophies based on specific frequency signals. Additionally, the robot could autonomously locate and move a police car box, demonstrating its ability to execute complex tasks and apply controlled force as needed, with ToF sensors aiding in proximity determination.

# Electrical Design

### Circuit Design


<div align="center">
    <img src="https://github.com/jakedonnini/MEAM_510_Robot/assets/24221155/106a62c3-d0fb-4411-8a3b-1b29d191e835" alt="Electrical Schematic">
</div>


The robot features two vive sensors with an amplifying circuit for each, H-bridge motor driver, a logic inverter to save a pin for the H-bridge, IR scanning amplifier, two time of flight sensors connected with I2C, two voltage regulators, two ESP32Cs and one ESP32S2. Each board communicates through UDP through the router in STA mode. We tried using I2C to communicate between the boards but we found that it interrupted our functions that required precision timing. We then tried ESPNOW but we were unable to get that to work with the WiFi functions. The only way we were able to get each ESP to talk to each other and still get good data from the sensors was with UDP.
We took many precautions to deal with noise in the circuit. On the main board with the motor driver the motors are wired directly to the battery and a voltage regulator with decoupling capacitors is used to power the 5V devices. The biggest concern with noise came from the IR trophy searching amplifier. Because the gain is so high it is very sensitive to any noise. We added a decoupling capacitor to the power inputs of each op-amp and twisted the wires of the two photodiodes to reduce noise. Additionally, an additional op-amp as a comparator with hysteresis was used at the output of each amplifier. This was done to make the path between high impedance inputs and outputs as small as possible to reduce the interference. Additionally, it gave a much cleaner signal for the ESP to detect even when the amplifier output is small.

### Processor architecture and code architecture

<div align="center">
    <img src="https://github.com/jakedonnini/MEAM_510_Robot/assets/24221155/7765a2ed-e128-4874-91b6-5e28b5d83056" alt="Processsing">
</div>

Starting at the top, the vive sensor took a long time to get working. We tried many different communication protocols but the only one that was able to send the positions of the two sensors was UDP. We found that an ESPC3 was not fast enough to send and collect the data so we went with an ESP32S2 and even then it could only transmit data at a frequency of 3 Hz or it would break.
The next ESP32C3 handled the time of flight distance sensors and the IR trophy scanner. I2C bus couldn’t be used while the IR sensor was sensing frequency because something in the I2C library was blocking so it was returning the wrong periods. We decide that when a search is happening the distance values are only sent every second or so and then a search is not happening the distances are sent continuously. This solved the problem and allowed us to keep both features without major reworks to the architecture. It also receives messages from the main board indicating whether it should be searching for real or fake trophies or stop searching.
The last board is where most decisions are made. The other two boards mainly send their data to this main board. Each action is treated as a state with flags to indicate which is active. Using the data from the other boards it can follow with the distance measurements and search with the servo position. During a search, the aim is to keep the position at 0 so the robot will rotate until it's close to 0 in which case it will drive forward. If it doesn’t see anything then it drives to the center and spins until it does. Using the vive data we can determine our position and angle. The move function uses this by taking a target coordinate from clicking on the map on the GUI and the robot will rotate until the angle matches and drive forward. If the angle stops matching then it rotates again to readjust until its at the destination. To find the police car, it gets the coordinates from the other UDP channel and runs the moveTo function on that target point until the distance sensor reads its close then it drives full power into it. Lastly, The wheels have a PID controller and the wall following implements a PD controller. 
The website is an interactive map of the field that displays the location of the police car and the two vive sensors in real time. The user can click on the map and the robot will autonomously navigate to that point. It displaces the readings from both distance sensors and statuses for each action. It features buttons to toggle between the different actions the robot can take during the competition.


# Mechinical Design
<img src="https://github.com/jakedonnini/MEAM_510_Robot/assets/24221155/6ed553b1-15ea-4360-82b6-2eda69b07031" width="45%"></img> <img src="https://github.com/jakedonnini/MEAM_510_Robot/assets/24221155/8984bc85-2eea-4d53-8594-965e93021dd9" width="45%"></img> 


# UI Design


<div align="center">
    <img src="https://github.com/jakedonnini/MEAM_510_Robot/assets/24221155/92533aa6-4708-4a2a-97cb-0e33ef75ecc2" alt="UIUpdated">
</div>




# Robot Final Design


## Top View
<img src="https://github.com/jakedonnini/MEAM_510_Robot/assets/24221155/6cf05e2a-1407-4638-840a-ac0282e26e56" width="30%"></img> <img src="https://github.com/jakedonnini/MEAM_510_Robot/assets/24221155/fd774aef-d0ac-41b4-994f-fe54eb430ce8" width="30%"></img> <img src="https://github.com/jakedonnini/MEAM_510_Robot/assets/24221155/af394804-f2d1-4609-84b7-f9270213f7c1" width="30%"></img> 

## Front View
<img src="https://github.com/jakedonnini/MEAM_510_Robot/assets/24221155/49e2d5d2-82d9-410e-bd17-3794a6fa6250" width="90%"></img> 


## Side View
![PXL_20231219_214635299](https://github.com/jakedonnini/MEAM_510_Robot/assets/24221155/f1166ba6-b639-443f-b510-34723e3bd3bc)


## Bottom View
![PXL_20231219_214743413](https://github.com/jakedonnini/MEAM_510_Robot/assets/24221155/b6f62fa5-8e3e-4cdb-866a-701edd8ade88)


## Closes Ups
<img src="https://github.com/jakedonnini/MEAM_510_Robot/assets/24221155/dd5edef2-572c-4ea0-9a77-e553f91c13c9" width="45%"></img> <img src="https://github.com/jakedonnini/MEAM_510_Robot/assets/24221155/68c2552a-c8a8-459e-b69d-5c0b69a9d88c" width="45%"></img> <img src="https://github.com/jakedonnini/MEAM_510_Robot/assets/24221155/3110d174-1c1b-4f28-8e01-1cbb7e9e8d6c" width="45%"></img> <img src="https://github.com/jakedonnini/MEAM_510_Robot/assets/24221155/582b417c-8b75-4134-85dd-f09d9d7f0a0e" width="45%"></img> 





# Areas for Improvement:

Servo Motor and Power Management:
The issue with servo motors consuming excessive current and causing microcontroller failures highlights a need for better power management. Implementing current limiting circuits, using more robust microcontrollers capable of handling higher currents, or choosing different servo motors could mitigate these issues.

Noise Management in Electrical Circuits:
While precautions were taken to manage noise, particularly in the IR trophy-searching amplifier, there’s room for further improvement in noise reduction techniques. Implementing more robust filtering methods or redesigning certain circuit elements to minimize electromagnetic interference could improve the accuracy and reliability of the sensors.









