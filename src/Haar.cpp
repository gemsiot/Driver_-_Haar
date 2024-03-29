//© 2023 Regents of the University of Minnesota. All rights reserved.

#include <Haar.h>

Haar::Haar(uint8_t talonPort_, uint8_t sensorPort_, uint8_t version): presSensor(), rhSensor()
{
	//Only update values if they are in range, otherwise stick with default values
	if(talonPort_ > 0) talonPort = talonPort_ - 1;
	else talonPort = 255; //Reset to null default if not in range
	if(sensorPort_ > 0) sensorPort = sensorPort_ - 1;
	else sensorPort = 255; //Reset to null default if not in range 
	sensorInterface = BusType::I2C; 
}

String Haar::begin(time_t time, bool &criticalFault, bool &fault)
{
	// Serial.println("HAAR - BEGIN"); //DEBUG!
	Wire.beginTransmission(ADR_DPS368);
	int errorBase = Wire.endTransmission();

	Wire.beginTransmission(ADR_DPS368_ALT);
	int errorAlt = Wire.endTransmission();

	if(errorBase == 0) {
		presSensor.begin(Wire, ADR_DPS368); //If address found at base address, use base addresses to initialize
		adrDPS368 = ADR_DPS368; //Update address
	}
	else if(errorAlt == 0) {
		presSensor.begin(Wire, ADR_DPS368_ALT); //If alt address passes, then use alt address to init
		adrDPS368 = ADR_DPS368; //Update address
	}
	else throwError(DPS368_INIT_ERROR | talonPortErrorCode); //If neither are found, throw error
	
	if(rhSensor.begin(ADR_SHT31) == false) {
		int initAttempt = rhSensor.begin(ADR_SHT31_ALT); //Try again with alt address
		// Serial.println("\tSHT31 Init Fail"); //DEBUG!
		if(initAttempt == false) throwError(SHT3X_INIT_ERROR | talonPortErrorCode); //If both the base address and alt address fail to init, then throw error
	} 
	
	// Wire.beginTransmission(0x76);
	// int error = Wire.endTransmission();
	// if(error != 0) {
	// 	// Serial.println("\tDPS368 Init Fail"); //DEBUG!
	// 	throwError(DPS368_INIT_ERROR | (error << 12) | talonPortErrorCode); //Error subtype = I2C error
	// }
	
	// Wire.beginTransmission(0x44);
	// int errorB = Wire.endTransmission();
	// ret = pres.measureTempOnce(temperature, oversampling);
	// Serial.print("INIT: ");
	// if(errorA == 0 || errorB == 0) Serial.println("PASS");
	// else {
	// 	Serial.print("ERR - ");
	// 	if(errorA != 0) {
	// 		Serial.print("A\t");
	// 		throwError(SHT3X_I2C_ERROR | (errorA << 12) | talonPortErrorCode); //Error subtype = I2C error
	// 	}
	// 	if(errorB != 0) Serial.print("B\t");
	// 	Serial.println("");
	// }
	return ""; //DEBUG!
}

String Haar::getMetadata()
{
	// Wire.beginTransmission(0x58); //Write to UUID range of EEPROM
	// Wire.write(0x98); //Point to start of UUID
	// int error = Wire.endTransmission();
	// // uint64_t uuid = 0;
	// String uuid = "";

	// if(error != 0) throwError(EEPROM_I2C_ERROR | error);
	// else {
	// 	uint8_t val = 0;
	// 	Wire.requestFrom(0x58, 8); //EEPROM address
	// 	for(int i = 0; i < 8; i++) {
	// 		val = Wire.read();//FIX! Wait for result??
	// 		// uuid = uuid | (val << (8 - i)); //Concatonate into full UUID
	// 		uuid = uuid + String(val, HEX); //Print out each hex byte
	// 		// Serial.print(Val, HEX); //Print each hex byte from left to right
	// 		// if(i < 7) Serial.print('-'); //Print formatting chracter, don't print on last pass
	// 		if(i < 7) uuid = uuid + "-"; //Print formatting chracter, don't print on last pass
	// 	}
	// }

	String metadata = "\"HAAR\":{";
	// if(error == 0) metadata = metadata + "\"SN\":\"" + uuid + "\","; //Append UUID only if read correctly, skip otherwise 
	metadata = metadata + "\"Hardware\":\"v" + String(version >> 4, HEX) + "." + String(version & 0x0F, HEX) + "\","; //Report version as modded BCD
	metadata = metadata + "\"Firmware\":\"" + FIRMWARE_VERSION + "\","; //Static firmware version 
	metadata = metadata + "\"Pos\":[" + getTalonPortString() + "," + getSensorPortString() + "]"; //Concatonate position 
	metadata = metadata + "}"; //CLOSE  
	return metadata; 
}

String Haar::getData(time_t time)
{
	float temperatureDPS368;
	float pressure;
	uint8_t oversampling = 7;
	int16_t ret;
	
	String output = "\"HAAR\":{"; //OPEN JSON BLOB
	String dps368Data = "\"DPS368\":{\"Temperature\":"; //Open dps368 substring
	String sht3xData = "\"SHT31\":{\"Temperature\":"; //Open SHT31 substring //FIX! How to deal with SHT31 vs SHT35?? Do we deal with it at all

	//lets the Dps368 perform a Single temperature measurement with the last (or standard) configuration
	//The result will be written to the paramerter temperature
	//ret = Dps368PressureSensor.measureTempOnce(temperature);
	//the commented line below does exactly the same as the one above, but you can also config the precision
	//oversampling can be a value from 0 to 7
	//the Dps 368 will perform 2^oversampling internal temperature measurements and combine them to one result with higher precision
	//measurements with higher precision take more time, consult datasheet for more information
	// Wire.beginTransmission(0x77);
	// int errorA = Wire.endTransmission();
	// Wire.beginTransmission(0x76);
	// int errorB = Wire.endTransmission();
	if(getSensorPort() != 0) { //If sensor detected
		bool dummy1;
		bool dummy2;
		begin(0, dummy1, dummy2); //DEBUG!
		ret = presSensor.measureTempOnce(temperatureDPS368, oversampling); //Measure temp
		if(ret == 0) { //If no error in read
			dps368Data = dps368Data + String(temperatureDPS368,2) + ","; //Append temp with 2 decimal points since resolution is 0.01°C, add comma
			// dps368Data = dps368Data + "Pressure" + String()
		}
		else {
			dps368Data = dps368Data + "null,"; //Append null as non-report 
			throwError(DPS368_READ_ERROR | 0x1000 | talonPortErrorCode | sensorPortErrorCode); //Error subtype = temp
			//FIX! Throw error
		}
		ret = presSensor.measurePressureOnce(pressure, oversampling); //Measure pressure 
		if(ret == 0) { //If no error in read
			dps368Data = dps368Data + "\"Pressure\":" + String(pressure/100.0,3); //Append pressure (divided from Pa to hPa, which is equal to mBar) with 3 decimal points since resolution is 0.002hPa
		}
		else {
			dps368Data = dps368Data + "\"Pressure\":null"; //Append null as non-report
			throwError(DPS368_READ_ERROR | 0x2000 | talonPortErrorCode | sensorPortErrorCode); //Error subtype = pres
			//FIX! Throw error
		}
		dps368Data = dps368Data + "}"; //Close DPS368 substring

		float temperatureSHT3x = rhSensor.readTemperature();
		float humidity = rhSensor.readHumidity();

		if(!isnan(temperatureSHT3x)) { //If no error in read
			sht3xData = sht3xData + String(temperatureSHT3x,2) + ","; //Append temp with 2 decimal points since resolution is 0.01°C, add comma
			// dps368Data = dps368Data + "Pressure" + String()
		}
		else {
			sht3xData = sht3xData + "null,"; //Append null as non-report 
			Wire.beginTransmission(adrDPS368);
			int error = Wire.endTransmission();
			if(error == 0) throwError(SHT3X_NAN_ERROR | 0x1000 | talonPortErrorCode | sensorPortErrorCode); //Error subtype = temp
			else throwError(SHT3X_I2C_ERROR | (error << 12) | talonPortErrorCode | sensorPortErrorCode); //Error subtype = I2C error
			
			//FIX! Throw error
		}

		if(!isnan(humidity)) { //If no error in read
			sht3xData = sht3xData + "\"Humidity\":" + String(humidity,2); //Append humidity with 2 decimal points since resolution is 0.01%
		}
		else {
			sht3xData = sht3xData + "\"Humidity\":null"; //Append null as non-report
			Wire.beginTransmission(adrDPS368);
			int error = Wire.endTransmission();
			if(error == 0) throwError(SHT3X_NAN_ERROR | 0x2000 | talonPortErrorCode | sensorPortErrorCode); //Error subtype = RH
			else throwError(SHT3X_I2C_ERROR | (error << 12) | talonPortErrorCode | sensorPortErrorCode); //Error subtype = I2C error
		}
		sht3xData = sht3xData + "}"; //Close SHT3x substring
	}
	else {
		throwError(FIND_FAIL); //Report failure to find
		dps368Data = dps368Data + "null,\"Pressure\":null}"; //Fill out null string
		sht3xData = sht3xData + "null,\"Pressure\":null}"; //Fill out null string
	}
	

	// Serial.print("TEMP: ");
	// if(errorA == 0 || errorB == 0) Serial.println(temperature); 
	// else {
	// 	Serial.print("ERR - ");
	// 	if(errorA != 0) Serial.print("A\t");
	// 	if(errorB != 0) Serial.print("B\t");
	// 	Serial.println("");
	// }
	
	output = output + dps368Data + "," + sht3xData + ",";
	output = output + "\"Pos\":[" + getTalonPortString() + "," + getSensorPortString() + "]"; //Concatonate position 
	output = output + "}"; //CLOSE JSON BLOB
	Serial.println(output); //DEBUG!
	return output;
}

bool Haar::isPresent() 
{ //FIX!
	Wire.beginTransmission(ADR_DPS368_ALT);
	int errorA = Wire.endTransmission();
	Wire.beginTransmission(ADR_DPS368);
	int errorB = Wire.endTransmission();
	Serial.print("HAAR TEST: "); //DEBUG!
	Serial.print(errorA);
	Serial.print("\t");
	Serial.println(errorB);
	if(errorA == 0 || errorB == 0) return true;
	else return false;
}

// void Haar::setTalonPort(uint8_t port)
// {
// 	// if(port_ > numPorts || port_ == 0) throwError(PORT_RANGE_ERROR | portErrorCode); //If commanded value is out of range, throw error 
// 	if(port > 4 || port == 0) throwError(TALON_PORT_RANGE_ERROR | talonPortErrorCode | sensorPortErrorCode); //If commanded value is out of range, throw error //FIX! How to deal with magic number? This is the number of ports on KESTREL, how do we know that??
// 	else { //If in range, update the port values
// 		talonPort = port - 1; //Set global port value in index counting
// 		talonPortErrorCode = (talonPort + 1) << 4; //Set port error code in rational counting 
// 	}
// }

// void Haar::setSensorPort(uint8_t port)
// {
// 	// if(port_ > numPorts || port_ == 0) throwError(PORT_RANGE_ERROR | portErrorCode); //If commanded value is out of range, throw error 
// 	if(port > 4 || port == 0) throwError(SENSOR_PORT_RANGE_ERROR | talonPortErrorCode | sensorPortErrorCode); //If commanded value is out of range, throw error //FIX! How to deal with magic number? This is the number of ports on KESTREL, how do we know that??
// 	else { //If in range, update the port values
// 		sensorPort = port - 1; //Set global port value in index counting
// 		sensorPortErrorCode = (sensorPort + 1); //Set port error code in rational counting 
// 	}
// }

// String Haar::getSensorPortString()
// {
// 	if(sensorPort >= 0 && sensorPort < 255) return String(sensorPort + 1); //If sensor port has been set //FIX max value
// 	else return "null";
// }

// String Haar::getTalonPortString()
// {
// 	if(talonPort >= 0 && talonPort < 255) return String(talonPort + 1); //If sensor port has been set //FIX max value
// 	else return "null";
// }

// int Haar::throwError(uint32_t error)
// {
// 	errors[(numErrors++) % MAX_NUM_ERRORS] = error; //Write error to the specified location in the error array
// 	if(numErrors > MAX_NUM_ERRORS) errorOverwrite = true; //Set flag if looping over previous errors 
// 	return numErrors;
// }

String Haar::getErrors()
{
	// if(numErrors > length && numErrors < MAX_NUM_ERRORS) { //Not overwritten, but array provided still too small
	// 	for(int i = 0; i < length; i++) { //Write as many as we can back
	// 		errorOutput[i] = error[i];
	// 	}
	// 	return -1; //Throw error for insufficnet array length
	// }
	// if(numErrors < length && numErrors < MAX_NUM_ERRORS) { //Not overwritten, provided array of good size (DESIRED)
	// 	for(int i = 0; i < numErrors; i++) { //Write all back into array 
	// 		errorOutput[i] = error[i];
	// 	}
	// 	return 0; //Return success indication
	// }
	String output = "\"HAAR\":{"; // OPEN JSON BLOB
	output = output + "\"CODES\":["; //Open codes pair

	for(int i = 0; i < min(MAX_NUM_ERRORS, numErrors); i++) { //Interate over used element of array without exceeding bounds
		output = output + "\"0x" + String(errors[i], HEX) + "\","; //Add each error code
		errors[i] = 0; //Clear errors as they are read
	}
	if(output.substring(output.length() - 1).equals(",")) {
		output = output.substring(0, output.length() - 1); //Trim trailing ','
	}
	output = output + "],"; //close codes pair
	output =  output + "\"OW\":"; //Open state pair
	if(numErrors > MAX_NUM_ERRORS) output = output + "1,"; //If overwritten, indicate the overwrite is true
	else output = output + "0,"; //Otherwise set it as clear
	output = output + "\"NUM\":" + String(numErrors) + ","; //Append number of errors
	output = output + "\"Pos\":[" + getTalonPortString() + "," + getSensorPortString() + "]"; //Concatonate position 
	output = output + "}"; //CLOSE JSON BLOB
	numErrors = 0; //Clear error count
	return output;

	// return -1; //Return fault if unknown cause 
}