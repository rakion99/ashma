#pragma once
#include  <iostream>
#include  <string>
#include  <windows.h>
#include  <tchar.h>

HANDLE CreatePipe();
void WaitForClient(HANDLE pipe);
bool SendData(HANDLE pipe, const char* message);
void GetInput(HANDLE pipe);
int main();
