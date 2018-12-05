#include <stdio.h>

#include "owl.h"
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>

#define MARKER_COUNT 2
#define SERVER_NAME "192.168.0.6"
// comment out to run alone
#define SLAVE_CLIENT_MODE 

void owl_print_error(const char *s, int n);

int main()
{
	OWLMarker markers[32];
	int tracker;

	size_t flags = 0;

#ifdef SLAVE_CLIENT_MODE
	flags |= OWL_SLAVE;
#endif

	if (owlInit(SERVER_NAME, flags) < 0) return 0;

	owlSetInteger(OWL_FRAME_BUFFER_SIZE, 0);
#ifndef SLAVE_CLIENT_MODE
	// create tracker 0
	tracker = 0;
	owlTrackeri(tracker, OWL_CREATE, OWL_POINT_TRACKER);

	// set markers
	for (int i = 0; i < MARKER_COUNT; i++)
		owlMarkeri(MARKER(tracker, i), OWL_SET_LED, i);

	// activate tracker
	owlTracker(tracker, OWL_ENABLE);

	// flush requests and check for errors
	if (!owlGetStatus())
	{
		owl_print_error("error in point tracker setup", owlGetError());
		return 0;
	}
#endif

	// set default frequency
	owlSetFloat(OWL_FREQUENCY, OWL_MAX_FREQUENCY);

	// start streaming
	owlSetInteger(OWL_STREAMING, OWL_ENABLE);

	// create time stamp
	SYSTEMTIME st;
	GetSystemTime(&st);
	printf("%d_%d_%d %d_%d_%d",
		st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);

	// name file output
	using namespace std;
	ofstream outputFile;
		std::string filename = "data/"+to_string(st.wDay)+"_"+to_string(st.wMonth)+"_"+to_string(st.wYear)+" "+to_string(st.wHour)+"_"+to_string(st.wMinute)+"_"+to_string(st.wSecond)+".csv";
	
	// create and open csv
	outputFile.open(filename);
	
	// write file headers
	outputFile << "Time (s)";
	for (int j = 0; j < MARKER_COUNT; j++) outputFile << ",,LED " << ("%d", j+1) << ",,";
	outputFile << std::endl;
	for (int j = 0; j < MARKER_COUNT; j++) outputFile << ",x," << "y," << "z,";
	outputFile << std::endl;

	// start timer
	auto start = std::chrono::high_resolution_clock::now();
	// main loop
	while (1)
	{
		int err;

		// get some markers
		int n = owlGetMarkers(markers, 32);

		// check for error
		if ((err = owlGetError()) != OWL_NO_ERROR)
		{
			owl_print_error("error", err);
			break;
		}

		// no data yet
		if (n == 0)
		{
			continue;
		}
		
		if (n > 0 && (markers[0].cond > 0))
		{
			// write time stamp to file
			auto finish = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> elapsed = finish - start;
			outputFile << ("%f", elapsed.count());
			// display data
			printf("%d marker(s):\n", n);
			for (int i = 0; i < n; i++)
			{
				if (markers[i].cond > 0) 
				{
					printf("%d) %f mm %f mm %f mm\n", i, markers[i].x, markers[i].y, markers[i].z);
					printf("\n");
				}

				// write data to file
				outputFile << "," << ("%f", markers[i].x) << "," << ("%f", markers[i].y) << "," << ("%f", markers[i].z) << ",";

				if (i == n - 1) 
				{
					outputFile << std::endl;
				}
			}
		}
	}

	// close the output file
	outputFile.close();

	// cleanup
	owlDone();
}

void owl_print_error(const char *s, int n)
{
	if (n < 0) printf("%s: %d\n", s, n);
	else if (n == OWL_NO_ERROR) printf("%s: No Error\n", s);
	else if (n == OWL_INVALID_VALUE) printf("%s: Invalid Value\n", s);
	else if (n == OWL_INVALID_ENUM) printf("%s: Invalid Enum\n", s);
	else if (n == OWL_INVALID_OPERATION) printf("%s: Invalid Operation\n", s);
	else printf("%s: 0x%x\n", s, n);
}
