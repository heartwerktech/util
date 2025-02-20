

# requirements:

- use platformio
- add to platformio.ini:
```
lib_deps = 
	https://github.com/me-no-dev/ESPAsyncWebServer
	links2004/WebSockets@^2.4.1
	bblanchon/ArduinoJson@^6.21.3
```


# usage:
- subclass ParameterServer + ParameterData
- add data folder to main directory, build + load file system.