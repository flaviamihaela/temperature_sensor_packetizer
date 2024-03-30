# temperature_sensor_packetizer

The code is a comprehensive application for interfacing with a microcontroller-based setup, involving temperature sensing, packet formation, and data storage/retrieval using EEPROM.

## Key Aspects:
 - Temperature Reading: Utilizes a temperature sensor connected via I2C to read temperature data. The temperature data is processed and stored in a custom packet structure.

 - Packet Handling: Defines a packet structure that includes source and destination MAC addresses, length, payload (containing the temperature data), and a Frame Check Sequence (FCS) for error checking. The payload structure within the packet is designed to hold a temperature sample along with additional data.

 - EEPROM Operations: Implements functions to write the packet data to EEPROM for persistent storage and read it back. This includes handling EEPROM addressing and acknowledge polling to ensure data integrity across successive write cycles.

- CRC Calculation: Employs a CRC (Cyclic Redundancy Check) calculation for the packet to ensure data integrity. This is particularly used in the FCS field of the packet.

 - User Interface: Uses a joystick for user input, allowing different operations like reading temperature, writing to EEPROM, and cycling through packet fields on an LCD display. Each operation is followed by a corresponding success message on the LCD.

 - I2C Communication: Configures and utilizes I2C communication for interfacing with the temperature sensor and EEPROM, including setting up the necessary GPIO pins and I2C parameters.

 - GPIO and Peripheral Configuration: Sets up General-Purpose Input/Output (GPIO) pins and other peripherals (like SPI for the LCD display, and I2C for sensor and EEPROM communication) required for the application.
