#pragma once
#include  <iostream>
#include  <string>
#include  <windows.h>
#include  <tchar.h>

HANDLE _CreatePipe();
void WaitForClient(HANDLE pipe);
bool SendData(HANDLE pipe, const char* message);
void GetInput(HANDLE pipe);
void ReadInput(void* const pipe);
int main_pipe();