# Automatic_4_zones_watering_system
A system for watering your plants when you are not home

## How it works 
Moisture sensors of the 4 zones deliver their values to the ESp32 board, once the level is below a threshold, the board opens the relay of that zone so the motor immersed in water delivers water through a small hose to the plant.

## parameters : 
- 4 zones, expantion is possible
- Moisture threshods are 1500 (aroud 3000 in air, around 900 in water)
- sleep cycle (every 3/4 days) for energy consumption optimization (TBD)

