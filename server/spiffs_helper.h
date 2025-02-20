#pragma once

#include <Arduino.h>
#include <SPIFFS.h>

bool _spiffsStarted = false;

void initFS()
{
  if (!_spiffsStarted)
  {
    if (!SPIFFS.begin())
      Serial.println("SPIFFS could not initialize");
    else
      _spiffsStarted = true;
  }
}

String readFile(const char *path)
{

  File file = SPIFFS.open(path);
  if (!file || file.isDirectory())
  {
    printf("ERROR in spiffs_helper: Failed to open file at path: %s\n", path);
    return String();
  }

  String fileContent;
  while (file.available())
  {
    fileContent = file.readStringUntil('\n');
    break;
  }
  printf("Reading file: %s = '%s'\n", path, fileContent.c_str());
  return fileContent;
}

void writeFile(const char *path, const char *message)
{
  printf("Writing file: %s\r\n", path);

  File file = SPIFFS.open(path, FILE_WRITE);
  if (!file)
  {
    printf("ERROR in spiffs_helper: Failed to open file at path: %s\n", path);
    return;
  }
  printf(file.print(message) ? "- file written\n" : "- write failed\n");
}