#ifdef WINDOWS

#include "selftest_local.h"


void Test_Berry_VarLifeSpan() {
	// reset whole device
	SIM_ClearOBK(0);

	CMD_ExecuteCommand("berry x = 5;", 0);
	CMD_ExecuteCommand("berry setChannel(2, x)", 0);

	SELFTEST_ASSERT_CHANNEL(2, 5);
	CMD_ExecuteCommand("berry x += 5;", 0);
	CMD_ExecuteCommand("berry setChannel(2, x)", 0);
	SELFTEST_ASSERT_CHANNEL(2, 10);
	CMD_ExecuteCommand("berry x += getChannel(2)", 0);
	CMD_ExecuteCommand("berry setChannel(2, x)", 0);
	SELFTEST_ASSERT_CHANNEL(2, 20);
	CMD_ExecuteCommand("berry x += 1; x += 1", 0);
	CMD_ExecuteCommand("berry setChannel(2, x)", 0);
	SELFTEST_ASSERT_CHANNEL(2, 22);
	CMD_ExecuteCommand("berry x += 1\n x += 1", 0);
	CMD_ExecuteCommand("berry setChannel(2, x)", 0);
	SELFTEST_ASSERT_CHANNEL(2, 24);
	CMD_ExecuteCommand("berry x += 1\n x += 1\nsetChannel(2, x)", 0);
	SELFTEST_ASSERT_CHANNEL(2, 26);
	CMD_ExecuteCommand("berry x += 1\n x += 1\nif 1 x += 2; end\nsetChannel(2, x)", 0);
	SELFTEST_ASSERT_CHANNEL(2, 30);
	CMD_ExecuteCommand("berry x += 1\n x += 1\nif 0 x += 2; else x += 3 end\nsetChannel(2, 1000+x)", 0);
	SELFTEST_ASSERT_CHANNEL(2, 1035);
	CMD_ExecuteCommand("berry if 5==5 x += 2; else x += 3 end\nsetChannel(2, 1000+x)", 0);
	SELFTEST_ASSERT_CHANNEL(2, 1037);
	
}
void Test_Berry_ChannelSet() {
    int i;

    // reset whole device
    SIM_ClearOBK(0);

    // Make sure channels start at 0
    CMD_ExecuteCommand("setChannel 1 0", 0);
    CMD_ExecuteCommand("setChannel 2 0", 0);
    
    SELFTEST_ASSERT_CHANNEL(1, 0);
    SELFTEST_ASSERT_CHANNEL(2, 0);

	int startStackSize = Berry_GetStackSizeTotal();
    // Run Berry code that sets channels
    CMD_ExecuteCommand("berry setChannel(1, 42)", 0);
    CMD_ExecuteCommand("berry setChannel(2, 123)", 0);
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
    
    // Verify the channels were set correctly
    SELFTEST_ASSERT_CHANNEL(1, 42);
    SELFTEST_ASSERT_CHANNEL(2, 123);
    
    // Test setting multiple channels in one script
    CMD_ExecuteCommand("berry setChannel(1, 99); setChannel(2, 88)", 0);
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
    
    // Verify the updated values
    SELFTEST_ASSERT_CHANNEL(1, 99);
    SELFTEST_ASSERT_CHANNEL(2, 88);

	CMD_ExecuteCommand("berry def give() return 7 end; setChannel(1, give() * 3)", 0);
	SELFTEST_ASSERT_CHANNEL(1, 21);
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);

	CMD_ExecuteCommand("berry i = 5; def test() i = 10 end; test(); setChannel(1, i)", 0);
	SELFTEST_ASSERT_CHANNEL(1, 10); // or 5 depending on scoping rules
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);


	CMD_ExecuteCommand("berry setChannel(5, int(\"213\")); ", 0);
	SELFTEST_ASSERT_CHANNEL(5, 213);
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
}

void Test_Berry_CancelThread() {
    int i;

    // reset whole device
    SIM_ClearOBK(0);

	int startStackSize = Berry_GetStackSizeTotal();
    // Make sure channels start at 0
    CMD_ExecuteCommand("setChannel 1 0", 0);
    CMD_ExecuteCommand("setChannel 2 0", 0);
    
    SELFTEST_ASSERT_CHANNEL(1, 0);
    SELFTEST_ASSERT_CHANNEL(2, 0);

    // Run Berry code that creates a delayed thread that would set channels
    // and stores the thread ID in a global variable
    CMD_ExecuteCommand("berry thread_id = setTimeout(def() setChannel(1, 42); setChannel(2, 123); end, 100); print(thread_id)", 0);
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
    
    // Now cancel the thread using the thread_id
    CMD_ExecuteCommand("berry cancel(thread_id)", 0);
    
    // Run scheduler long enough for the original delay to have completed if it wasn't cancelled
    for (i = 0; i < 20; i++) {
        Berry_RunThreads(10);
    }
    
    // Verify the channels were NOT set (they should still be 0)
    SELFTEST_ASSERT_CHANNEL(1, 0);
    SELFTEST_ASSERT_CHANNEL(2, 0);
    
    // Now let's test the positive case - create a thread and let it complete
    CMD_ExecuteCommand("berry setTimeout(def() setChannel(1, 99); setChannel(2, 88); end, 50)", 0);
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
    
    // Run scheduler to let the thread complete
    for (i = 0; i < 20; i++) {
        Berry_RunThreads(10);
    }
    
    // Verify the channels WERE set this time
    SELFTEST_ASSERT_CHANNEL(1, 99);
    SELFTEST_ASSERT_CHANNEL(2, 88);

	// One more test
	CMD_ExecuteCommand("berry setTimeout(def() addChannel(1, 1); addChannel(2, 2); end, 50)", 0);
	// Run scheduler to let the thread complete
	for (i = 0; i < 20; i++) {
		Berry_RunThreads(10);
	}
	// Verify the channels were added
	SELFTEST_ASSERT_CHANNEL(1, 100);
	SELFTEST_ASSERT_CHANNEL(2, 90);
	// twice
	CMD_ExecuteCommand("berry setTimeout(def() addChannel(1, 1); addChannel(2, 2); end, 50)", 0);
	CMD_ExecuteCommand("berry setTimeout(def() addChannel(1, 1); addChannel(2, 2); end, 50)", 0);
	// Run scheduler to let the thread complete
	for (i = 0; i < 20; i++) {
		Berry_RunThreads(10);
	}
	SELFTEST_ASSERT_CHANNEL(1, 102);
	SELFTEST_ASSERT_CHANNEL(2, 94);
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);

}


void Test_Berry_SetInterval() {
	int i;

	// reset whole device
	SIM_ClearOBK(0);

	// Make sure channels start at 0
	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	CMD_ExecuteCommand("berry thread_id = setInterval(def() addChannel(1, 1); end, 100);", 0);
	// time 100ms, run for 101 ms, so be sure that it fired
	Berry_RunThreads(101);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	Berry_RunThreads(101);
	SELFTEST_ASSERT_CHANNEL(1, 2);
	Berry_RunThreads(101);
	SELFTEST_ASSERT_CHANNEL(1, 3);
	Berry_RunThreads(101);
	SELFTEST_ASSERT_CHANNEL(1, 4);
	Berry_RunThreads(101);
	SELFTEST_ASSERT_CHANNEL(1, 5);
	Berry_RunThreads(101);
	SELFTEST_ASSERT_CHANNEL(1, 6);
	Berry_RunThreads(102);
	SELFTEST_ASSERT_CHANNEL(1, 7);
	CMD_ExecuteCommand("berry cancel(thread_id);", 0);
	for (int i = 0; i < 10; i++) {
		Berry_RunThreads(102);
		SELFTEST_ASSERT_CHANNEL(1, 7);
	}

}
void Test_Berry_SetTimeout() {
	int i;

	// reset whole device
	SIM_ClearOBK(0);

	// Make sure channels start at 0
	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	CMD_ExecuteCommand("berry thread_id = setTimeout(def() addChannel(1, 1); end, 100);", 0);
	// time 100ms, run for 101 ms, so be sure that it fired
	Berry_RunThreads(101);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	Berry_RunThreads(101);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	Berry_RunThreads(101);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	Berry_RunThreads(101);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	Berry_RunThreads(101);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	Berry_RunThreads(101);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	Berry_RunThreads(102);
	SELFTEST_ASSERT_CHANNEL(1, 1);


}
void Test_Berry_Import() {
    int i;

    // reset whole device
    SIM_ClearOBK(0);
		CMD_ExecuteCommand("lfs_format", 0);

    // Create an autoexec.be file with a proper module pattern
    Test_FakeHTTPClientPacket_POST("api/lfs/autoexec.be",
        "autoexec = module('autoexec')\n"
        "\n"
        "# Add functions to the module\n"
        "autoexec.set_test_channels = def()\n"
        "  setChannel(1, 42)\n"
        "  setChannel(2, 84)\n"
        "end\n"
        "\n"
        "# This value can be accessed as autoexec.TEST_VALUE\n"
        "autoexec.TEST_VALUE = 123\n"
        "\n"
        "# Berry modules must return the module object\n"
        "return autoexec\n");

    // Make sure channels start at 0
    CMD_ExecuteCommand("setChannel 1 0", 0);
    CMD_ExecuteCommand("setChannel 2 0", 0);
    
    SELFTEST_ASSERT_CHANNEL(1, 0);
    SELFTEST_ASSERT_CHANNEL(2, 0);

    // Import autoexec module and call the function
    CMD_ExecuteCommand("berry import autoexec; autoexec.set_test_channels()", 0);
    
    // Verify the channels were set correctly by the imported module
    SELFTEST_ASSERT_CHANNEL(1, 42);
    SELFTEST_ASSERT_CHANNEL(2, 84);
    
    // Test accessing a module constant
    CMD_ExecuteCommand("berry import autoexec; setChannel(3, autoexec.TEST_VALUE)", 0);
    
    // Verify the constant from the module was correctly accessed
    SELFTEST_ASSERT_CHANNEL(3, 123);
}
void Test_Berry_Click() {
	int i;

	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	CMD_ExecuteCommand("setChannel 5 0", 0);
	Test_FakeHTTPClientPacket_POST("api/lfs/autoexec.be",
		"addEventHandler(\"OnClick\", 5, def(arg)\n"
		"	runCmd(\"addChannel 5 1\") \n"
		"end)\n");
	// in physical device, it should not be needed, it should run on its own
	CMD_ExecuteCommand("startScript autoexec.be", 0);

	SELFTEST_ASSERT_CHANNEL(5, 0);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_PIN_ONCLICK, 5, 0);
	SELFTEST_ASSERT_CHANNEL(5, 1);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_PIN_ONCLICK, 5, 0);
	SELFTEST_ASSERT_CHANNEL(5, 2);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_PIN_ONCLICK, 5, 0);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_PIN_ONCLICK, 5, 0);
	SELFTEST_ASSERT_CHANNEL(5, 4);
}
void Test_Berry_Click_And_Timeout() {
	int i;

	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	CMD_ExecuteCommand("setChannel 5 0", 0);
	Test_FakeHTTPClientPacket_POST("api/lfs/autoexec.be",
		"def myFunc2()\n"
		"	runCmd(\"addChannel 5 1\") \n"
		"end\n"
		"addEventHandler(\"OnClick\", 5, def(arg)\n"
		"	runCmd(\"addChannel 5 1\") \n"
		"	setTimeout(myFunc2, 1) \n"
		"end)\n");
	// in physical device, it should not be needed, it should run on its own
	CMD_ExecuteCommand("startScript autoexec.be", 0);

	SELFTEST_ASSERT_CHANNEL(5, 0);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_PIN_ONCLICK, 5, 0);
	SELFTEST_ASSERT_CHANNEL(5, 1);
	// wait 2 seconds for setTimeout to call myFunc2
	Sim_RunSeconds(2, false);
	SELFTEST_ASSERT_CHANNEL(5, 2);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(5, 2);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_PIN_ONCLICK, 5, 0);
	SELFTEST_ASSERT_CHANNEL(5, 3);
	// wait 2 seconds for setTimeout to call myFunc2
	Sim_RunSeconds(2, false);
	SELFTEST_ASSERT_CHANNEL(5, 4);
	Sim_RunSeconds(10, false);
	SELFTEST_ASSERT_CHANNEL(5, 4);
}
void Test_Berry_Import_Autorun() {
	int i;

	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	// Create an autoexec.be file with a proper module pattern
	Test_FakeHTTPClientPacket_POST("api/lfs/autoexec.be",
		"autoexec = module('autoexec')\n"
		"\n"
		"# Add functions to the module\n"
		"autoexec.init = def()\n"
		"  setChannel(1, 42)\n"
		"  setChannel(2, 84)\n"
		"end\n"
		"\n"
		"autoexec.init()\n"
		"\n"
		"# Berry modules must return the module object\n"
		"return autoexec\n");

	// Make sure channels start at 0
	CMD_ExecuteCommand("setChannel 1 0", 0);
	CMD_ExecuteCommand("setChannel 2 0", 0);

	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);

	// Import autoexec module and call the function
	CMD_ExecuteCommand("berry import autoexec", 0);

	// Verify the channels were set correctly by the imported module
	SELFTEST_ASSERT_CHANNEL(1, 42);
	SELFTEST_ASSERT_CHANNEL(2, 84);

	CMD_ExecuteCommand("setChannel 1 0", 0);
	CMD_ExecuteCommand("setChannel 2 0", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);

	// Import autoexec module and call the function
	CMD_ExecuteCommand("berry import autoexec", 0);
	// this will not reset them
	SELFTEST_ASSERT_CHANNEL(1, 0);
	SELFTEST_ASSERT_CHANNEL(2, 0);
}
void Test_Berry_Fibonacci() {
	int i, n = 5;
	long long result = 1;

	// Reset the whole device (for demo)
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	// Create an autoexec.be file with the Fibonacci sequence generator using if and recursion
	Test_FakeHTTPClientPacket_POST("api/lfs/tester.be",
		"tester = module('tester')\n"
		"\n"
		"tester.fib = def(n)\n"
		"	if n <= 1 return n end\n"
		"	return tester.fib(n - 1) + tester.fib(n - 2)\n"
		"end\n"
		"\n"
		"# Berry modules must return the module object\n"
		"return tester\n");

	// Make sure no channels are set before testing
	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);

	// Import the autoexec module and call the Fibonacci function
	CMD_ExecuteCommand("berry import tester; setChannel(1,tester.fib(10))", 0);
	// Verify the result of Fibonacci sequence 
	SELFTEST_ASSERT_CHANNEL(1, 55);
	// Import the autoexec module and call the Fibonacci function
	CMD_ExecuteCommand("berry import tester; setChannel(1,tester.fib(11))", 0);
	// Verify the result of Fibonacci sequence 
	SELFTEST_ASSERT_CHANNEL(1, 89);
}

void Test_Berry_ThreadCleanup() {
    int i;

    // reset whole device
    SIM_ClearOBK(0);
		CMD_ExecuteCommand("lfs_format", 0);

    // Clear all channels
    CMD_ExecuteCommand("setChannel 1 0", 0);
    CMD_ExecuteCommand("setChannel 2 0", 0);
    CMD_ExecuteCommand("setChannel 3 0", 0);
    
    SELFTEST_ASSERT_CHANNEL(1, 0);
    SELFTEST_ASSERT_CHANNEL(2, 0);
    SELFTEST_ASSERT_CHANNEL(3, 0);

    // First run a Berry script that completes quickly
    CMD_ExecuteCommand("berry setTimeout(def() setChannel(1, 42); end, 10)", 0);
    
    // Run the scheduler to let the Berry script complete
    for (i = 0; i < 5; i++) {
        Berry_RunThreads(10);
    }
    
    // Verify the Berry script did run
    SELFTEST_ASSERT_CHANNEL(1, 42);
    
    // Now create a script file that should run normally as an SVM script
    Test_FakeHTTPClientPacket_POST("api/lfs/testSVMScript.txt",
        "setChannel 2 123\n"
        "delay_ms 50\n"        // Add a delay to ensure the thread stays active
        "setChannel 3 456\n");

    // Run the SVM script - this should use a thread from the pool
    // If the thread still has isBerry=true, this will execute differently
    CMD_ExecuteCommand("startScript testSVMScript.txt", 0);
    
    // Let the script start running
		Berry_RunThreads(20);
		SVM_RunThreads(20);
    
    // Channel 2 should be set already (before the delay)
    SELFTEST_ASSERT_CHANNEL(2, 123);
    
    // Channel 3 should not be set yet (after the delay)
    SELFTEST_ASSERT_CHANNEL(3, 0);
    
    // Now let the script complete
    for (i = 0; i < 10; i++) {
        Berry_RunThreads(10);
		SVM_RunThreads(10);
    }
    
    // Now channel 3 should be set
    SELFTEST_ASSERT_CHANNEL(3, 456);
    
    // Run one more Berry script to confirm both types still work
    CMD_ExecuteCommand("berry setChannel(1, 789)", 0);
    SELFTEST_ASSERT_CHANNEL(1, 789);
}
void Test_Berry_AutoloadModule() {
    // reset whole device
    SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	int startStackSize = Berry_GetStackSizeTotal();
    // Create a Berry module file
    Test_FakeHTTPClientPacket_POST("api/lfs/autoexec.be",
        "autoexec = module('autoexec')\n"
        "\n"
        "autoexec.init = def (self)\n"
        "  print('Hello from autoexec.be')\n"
        "  setChannel(5, 42) # Set a channel so we can verify init ran\n"
        "  return self\n"
        "end\n"
        "\n"
        "return autoexec\n");
    
    // Create an autoexec.txt file that imports the module
    Test_FakeHTTPClientPacket_POST("api/lfs/autoexec.txt",
        "berry import autoexec\n");
    
    // Make sure channel starts at 0
    CMD_ExecuteCommand("setChannel 5 0", 0);
    SELFTEST_ASSERT_CHANNEL(5, 0);
    
    // Simulate a device restart by running the autoexec.txt script
    CMD_ExecuteCommand("startScript autoexec.txt", 0);
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
    
    // Run scheduler to let any scripts complete
    for (int i = 0; i < 5; i++) {
        Berry_RunThreads(10);
		SVM_RunThreads(10);
    }
    
    // Verify that the Berry module was loaded and initialized (it should have set channel 5 to 42)
    SELFTEST_ASSERT_CHANNEL(5, 42);
    
    // Test that we can now use the module functions directly
    CMD_ExecuteCommand("berry import autoexec; autoexec.init(); setChannel(6, 99)", 0);
    SELFTEST_ASSERT_CHANNEL(6, 99);
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
}

void Test_Berry_StartScriptShortcut() {
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	// Create a Berry module file
	Test_FakeHTTPClientPacket_POST("api/lfs/test.be",
		"def mySample()\n"
		"  print('Hello from test.be')\n"
		"  setChannel(5, 2025) # Set a channel so we can verify init ran\n"
		"end\n"
		"\n"
		"mySample()\n");

	// Make sure channel starts at 0
	CMD_ExecuteCommand("setChannel 5 0", 0);
	SELFTEST_ASSERT_CHANNEL(5, 0);

	// Simulate a device restart by running the autoexec.txt script
	CMD_ExecuteCommand("startScript test.be", 0);

	// Run scheduler to let any scripts complete
	for (int i = 0; i < 5; i++) {
		Berry_RunThreads(10);
	}

	// Verify that the Berry module was loaded and initialized (it should have set channel 5 to 2025)
	SELFTEST_ASSERT_CHANNEL(5, 2025);
}

void Test_Berry_StartScriptShortcut2() {

	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	// Make sure channel starts at 0
	CMD_ExecuteCommand("setChannel 5 0", 0);
	SELFTEST_ASSERT_CHANNEL(5, 0);

	// Create a Berry module file
	Test_FakeHTTPClientPacket_POST("api/lfs/test.be",
		"addChannel(5,1);\n");
	CMD_ExecuteCommand("startScript test.be", 0);
	SELFTEST_ASSERT_CHANNEL(5, 1);
	// wont work
	//CMD_ExecuteCommand("startScript test.be", 0);
	//SELFTEST_ASSERT_CHANNEL(5, 2);

}

void Test_Berry_PassArg() {
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	int startStackSize = Berry_GetStackSizeTotal();
	// Create a Berry module file
	Test_FakeHTTPClientPacket_POST("api/lfs/test.be",
		"def mySample(x)\n"
		"  print('Hello from test.be')\n"
		"  setChannel(5, x) # Set a channel so we can verify init ran\n"
		"end\n"
		"\n"
		"mySample(15)\n");

	// Make sure channel starts at 0
	CMD_ExecuteCommand("setChannel 5 0", 0);
	SELFTEST_ASSERT_CHANNEL(5, 0);

	// Simulate a device restart by running the autoexec.txt script
	CMD_ExecuteCommand("startScript test.be", 0);

	// Run scheduler to let any scripts complete
	for (int i = 0; i < 5; i++) {
		Berry_RunThreads(10);
	}

	// Verify that the Berry module was loaded and initialized (it should have set channel 5 to 15)
	SELFTEST_ASSERT_CHANNEL(5, 15);
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
}



void Test_Berry_PassArgFromCommand() {
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	int startStackSize = Berry_GetStackSizeTotal();
	// Make sure channel starts at 0
	CMD_ExecuteCommand("setChannel 5 0", 0);
	SELFTEST_ASSERT_CHANNEL(5, 0);

	// Simulate a device restart by running the autoexec.txt script
	CMD_ExecuteCommand("berry def mySample(x) addChannel(5, x) end", 0);
	CMD_ExecuteCommand("berry mySample(15)", 0);
	SELFTEST_ASSERT_CHANNEL(5, 15);
	CMD_ExecuteCommand("berry mySample(15)", 0);
	SELFTEST_ASSERT_CHANNEL(5, 30);
	CMD_ExecuteCommand("berry mySample(2)", 0);
	SELFTEST_ASSERT_CHANNEL(5, 32);
	CMD_ExecuteCommand("berry mySample(-5)", 0);
	SELFTEST_ASSERT_CHANNEL(5, 27);
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
}



void Test_Berry_PassArgFromCommandWithoutModule() {
	/*
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	// Create a Berry module file
	Test_FakeHTTPClientPacket_POST("api/lfs/test.be",
		"def mySample(x)\n"
		"  print('Hello from test.be')\n"
		"  addChannel(5, x) # Set a channel so we can verify init ran\n"
		"end\n\n");

	// Make sure channel starts at 0
	CMD_ExecuteCommand("setChannel 5 0", 0);
	SELFTEST_ASSERT_CHANNEL(5, 0);

	// Simulate a device restart by running the autoexec.txt script
	CMD_ExecuteCommand("berry import test", 0);
	CMD_ExecuteCommand("berry mySample(2)", 0);
	SELFTEST_ASSERT_CHANNEL(5, 2);
	CMD_ExecuteCommand("berry mySample(2)", 0);
	SELFTEST_ASSERT_CHANNEL(5, 4);
	CMD_ExecuteCommand("berry mySample(2)", 0);
	SELFTEST_ASSERT_CHANNEL(5, 6);
	*/
}
void Test_Berry_PassArgFromCommandWithModule() {
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	int startStackSize = Berry_GetStackSizeTotal();

	// Create a Berry module file
	Test_FakeHTTPClientPacket_POST("api/lfs/test.be",
		"test = module('test')\n"
		"\n"
		"# Add functions to the module\n"
		"test.mySample = def(x)\n"
		"  addChannel(5, x) # Set a channel so we can verify init ran\n"
		"end\n"
		"\n"
		"return test\n");

	// Make sure channel starts at 0
	CMD_ExecuteCommand("setChannel 5 0", 0);
	SELFTEST_ASSERT_CHANNEL(5, 0);

	// Simulate a device restart by running the autoexec.txt script
	CMD_ExecuteCommand("berry import test; test.mySample(2)", 0);
	SELFTEST_ASSERT_CHANNEL(5, 2);
	CMD_ExecuteCommand("berry import test; test.mySample(2)", 0);
	SELFTEST_ASSERT_CHANNEL(5, 4);
	CMD_ExecuteCommand("berry import test; test.mySample(2)", 0);
	SELFTEST_ASSERT_CHANNEL(5, 6);
	CMD_ExecuteCommand("berry test.mySample(2)", 0);
	SELFTEST_ASSERT_CHANNEL(5, 8);
	CMD_ExecuteCommand("berry test.mySample(2)", 0);
	SELFTEST_ASSERT_CHANNEL(5, 10);
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
}

void Test_Berry_AutoexecAndBerry() {
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	int startStackSize = Berry_GetStackSizeTotal();

	// Create a Berry module file
	Test_FakeHTTPClientPacket_POST("api/lfs/test.be",
		"test = module('test')\n"
		"\n"
		"# Add functions to the module\n"
		"test.mySample = def(x)\n"
		"  addChannel(5, x) # Set a channel so we can verify init ran\n"
		"end\n"
		"\n"
		"return test\n");


	// Make sure channel starts at 0
	CMD_ExecuteCommand("setChannel 5 0", 0);
	SELFTEST_ASSERT_CHANNEL(5, 0);

	Test_FakeHTTPClientPacket_POST("api/lfs/autoexecTest.bat",
		"berry import test\n"
		"addRepeatingEvent 5 -1 berry test.mySample(2)\n");

	CMD_ExecuteCommand("startScript autoexecTest.bat", 0);
	Sim_RunSeconds(6, false);
	// it ran once
	SELFTEST_ASSERT_CHANNEL(5, 2);
	Sim_RunSeconds(6, false);
	// it ran once
	SELFTEST_ASSERT_CHANNEL(5, 4);
	Sim_RunSeconds(6, false);
	// it ran once
	SELFTEST_ASSERT_CHANNEL(5, 6);
	Sim_RunSeconds(6, false);
	// it ran once
	SELFTEST_ASSERT_CHANNEL(5, 8);

	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
}

void Test_Berry_FileSystem() {
	// reset whole device
	SIM_ClearOBK(0);

	CMD_ExecuteCommand("lfs_format", 0);

	int startStackSize = Berry_GetStackSizeTotal();

	// Create a Berry module file
	Test_FakeHTTPClientPacket_POST("api/lfs/test.be",
		"test = module('test')\n"
		"\n"
		"# Add functions to the module\n"
		"test.mySample = def()\n"
		"  f = open('test.txt', 'w') \n"
		"  f.write('foo bar')\n"
		"  f.close()\n"
		"end\n"
		"\n"
		"return test\n");

	// Simulate a device restart by running the autoexec.txt script
	CMD_ExecuteCommand("berry import test; test.mySample()", 0);

	Test_FakeHTTPClientPacket_GET("api/run/test.txt");
	SELFTEST_ASSERT_HTML_REPLY("foo bar");


	// Create a Berry module file
	Test_FakeHTTPClientPacket_POST("api/lfs/test2.be",
		"test2 = module('test2')\n"
		"\n"
		"# Add functions to the module\n"
		"test2.mySample = def()\n"
		"  f = open('test.txt', 'a') \n"
		"  f.write(' hey')\n"
		"  f.close()\n"
		"end\n"
		"\n"
		"return test2\n");
	CMD_ExecuteCommand("berry import test2; test2.mySample()", 0);
	Test_FakeHTTPClientPacket_GET("api/run/test.txt");
	SELFTEST_ASSERT_HTML_REPLY("foo bar hey");
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
}

// 
void Test_Berry_AddChangeHandler() {
	int i;

	// reset whole device
	SIM_ClearOBK(0);

	int startStackSize = Berry_GetStackSizeTotal();
	// Make sure channels start at 0
	CMD_ExecuteCommand("setChannel 1 0", 0);

	SELFTEST_ASSERT_CHANNEL(1, 0);
	CMD_ExecuteCommand("berry addChangeHandler(\"Channel3\", \"=\", 1, def()\n"
		"setChannel(1, 1) \n"
		"end)",0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	// addChangeHandler will fire
	CMD_ExecuteCommand("setChannel 3 1", 0);
	Berry_RunThreads(1);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	// if it was already 1, and we set 1, it will not fire
	CMD_ExecuteCommand("setChannel 3 1", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	// back to 0 - will not fire
	CMD_ExecuteCommand("setChannel 3 0", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	// addChangeHandler will fire
	CMD_ExecuteCommand("setChannel 3 1", 0);
	Berry_RunThreads(1);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	// back to 0 - will not fire
	CMD_ExecuteCommand("setChannel 3 0", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	// addChangeHandler will fire
	CMD_ExecuteCommand("setChannel 3 1", 0);
	Berry_RunThreads(1);
	SELFTEST_ASSERT_CHANNEL(1, 1);
	CMD_ExecuteCommand("setChannel 1 0", 0);
	SELFTEST_ASSERT_CHANNEL(1, 0);


	SELFTEST_ASSERT_CHANNEL(1, 0);
	CMD_ExecuteCommand("berry addEventHandler(\"Channel3\", def(x)\n"
		"setChannel(1, x) \n"
		"end)", 0);

	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_CHANGE_CHANNEL0+3, 431, 0); // 0 is unused
	SELFTEST_ASSERT_CHANNEL(1, 431);


	SELFTEST_ASSERT_CHANNEL(2, 0);
	CMD_ExecuteCommand("berry addChangeHandler(\"Channel5\", \">\", 5, def()\n"
		"addChannel(2, 1) \n"
		"end)", 0);
	CMD_ExecuteCommand("setChannel 5 0", 0);
	Berry_RunThreads(1);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	CMD_ExecuteCommand("setChannel 5 2", 0);
	Berry_RunThreads(1);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	CMD_ExecuteCommand("setChannel 5 4", 0);
	Berry_RunThreads(1);
	SELFTEST_ASSERT_CHANNEL(2, 0);
	CMD_ExecuteCommand("setChannel 5 6", 0);
	Berry_RunThreads(1);
	SELFTEST_ASSERT_CHANNEL(2, 1);
	CMD_ExecuteCommand("setChannel 5 7", 0);
	Berry_RunThreads(1);
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
	// TODO - it's triggered now, not as I expected....
	/*
	SELFTEST_ASSERT_CHANNEL(2, 1);
	CMD_ExecuteCommand("setChannel 5 8", 0);
	Berry_RunThreads(1);
	SELFTEST_ASSERT_CHANNEL(2, 1);
	CMD_ExecuteCommand("setChannel 5 7", 0);
	Berry_RunThreads(1);
	SELFTEST_ASSERT_CHANNEL(2, 1);
	CMD_ExecuteCommand("setChannel 5 6", 0);
	Berry_RunThreads(1);
	SELFTEST_ASSERT_CHANNEL(2, 1);
	CMD_ExecuteCommand("setChannel 5 66", 0);
	Berry_RunThreads(1);
	SELFTEST_ASSERT_CHANNEL(2, 1);
	// finally goes <= 5
	CMD_ExecuteCommand("setChannel 5 4", 0);
	Berry_RunThreads(1);
	SELFTEST_ASSERT_CHANNEL(2, 1);
	// now will fire
	CMD_ExecuteCommand("setChannel 5 6", 0);
	Berry_RunThreads(1);
	SELFTEST_ASSERT_CHANNEL(2, 2);
	*/
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
}
void Test_Berry_CommandRunner() {
	int i;

	// reset whole device
	SIM_ClearOBK(0);

	// Make sure channels start at 0
	CMD_ExecuteCommand("setChannel 1 0", 0);

	SELFTEST_ASSERT_CHANNEL(1, 0);
	CMD_ExecuteCommand("berry runCmd(\"setChannel 1 123\")", 0);
	SELFTEST_ASSERT_CHANNEL(1, 123);
	CMD_ExecuteCommand("berry runCmd(\"addChannel 1 7\")", 0);
	SELFTEST_ASSERT_CHANNEL(1, 130);
	CMD_ExecuteCommand("berry runCmd(\"addChannel 1 -7\")", 0);
	SELFTEST_ASSERT_CHANNEL(1, 123);

	CMD_ExecuteCommand("berry runCmd(\"MQTTHost 192.168.0.213\")", 0);
	SELFTEST_ASSERT_STRING("192.168.0.213", CFG_GetMQTTHost());
	CMD_ExecuteCommand("berry runCmd(\"MQTTHost 192.168.0.123\")", 0);
	SELFTEST_ASSERT_STRING("192.168.0.123", CFG_GetMQTTHost());
	CMD_ExecuteCommand("berry runCmd(\"MQTTHost 192.168.1.5\")", 0);
	SELFTEST_ASSERT_STRING("192.168.1.5", CFG_GetMQTTHost());
	// berry can concat strings
	CMD_ExecuteCommand("berry runCmd(\"MQTTHost \" + \"1.2.3.4\")", 0);
	SELFTEST_ASSERT_STRING("1.2.3.4", CFG_GetMQTTHost());
}
void Test_Berry_TuyaMCU() {
	int i;

	// reset whole device
	SIM_ClearOBK(0);

	SIM_UART_InitReceiveRingBuffer(2048);
	int startStackSize = Berry_GetStackSizeTotal();
	CMD_ExecuteCommand("startDriver TuyaMCU", 0);

	// per-dpID handler
	CMD_ExecuteCommand("setChannel 1 0", 0);
	CMD_ExecuteCommand("berry addEventHandler(\"OnDP\", 2, def(value)\n"
		"setChannel(1, value) \n"
		"end)", 0);

	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
	SELFTEST_ASSERT_CHANNEL(1, 0);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_ON_DP, 2, 123);
	SELFTEST_ASSERT_CHANNEL(1, 123);
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_ON_DP, 2, 564);
	SELFTEST_ASSERT_CHANNEL(1, 564);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_ON_DP, 1, 333);
	SELFTEST_ASSERT_CHANNEL(1, 564);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_ON_DP, 20, 555);
	SELFTEST_ASSERT_CHANNEL(1, 564);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_ON_DP, 22, 555);
	SELFTEST_ASSERT_CHANNEL(1, 564);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_ON_DP, 2, 123);
	SELFTEST_ASSERT_CHANNEL(1, 123);
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
	for (int i = 0; i < 15; i++) {
		int currentStackStart = Berry_GetStackSizeCurrent();
		int x = i * 15;
		printf("Loop start %i - Berry stack size: %i\n", i, Berry_GetStackSizeTotal());
		CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_ON_DP, 2, x);
		SELFTEST_ASSERT_CHANNEL(1, x);
		printf("Loop end %i - Berry stack size: %i\n", i, Berry_GetStackSizeTotal());
		SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
		int currentStackEnd = Berry_GetStackSizeCurrent();
		SELFTEST_ASSERT(currentStackStart== currentStackEnd);
	}

	// global handler
	CMD_ExecuteCommand("setChannel 1 0", 0);
	CMD_ExecuteCommand("berry addEventHandler(\"OnDP\", def(id, value)\n"
		"setChannel(5, id*10000+value) \n"
		"end)", 0);
	CMD_ExecuteCommand("setChannel 5 0", 0);
	Sim_RunFrames(100, false);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_ON_DP, 2, 123);
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
	SELFTEST_ASSERT_CHANNEL(5, 2 * 10000 + 123);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_ON_DP, 15, 123);
	SELFTEST_ASSERT_CHANNEL(5, 15 * 10000 + 123);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_ON_DP, 15, 555);
	SELFTEST_ASSERT_CHANNEL(5, 15 * 10000 + 555);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_ON_DP, 1, 555);
	SELFTEST_ASSERT_CHANNEL(5, 1 * 10000 + 555);
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);


	// This packet sets dpID 2 of type Value to 100
	CMD_ExecuteCommand("uartFakeHex 55AA0307000802020004000000647D", 0);
	Sim_RunFrames(100, false);
	SELFTEST_ASSERT_CHANNEL(1, 100);
	SELFTEST_ASSERT_CHANNEL(5, 2 * 10000 + 100);
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);

	for (int tr = 0; tr < 20; tr++) {
		printf("Berry stack size: %i\n",Berry_GetStackSizeTotal());
		// This packet sets dpID 2 of type Value to 90
		CMD_ExecuteCommand("uartFakeHex 55AA03070008020200040000005A73", 0);
		Sim_RunFrames(100, false);
		SELFTEST_ASSERT_CHANNEL(1, 90);
		SELFTEST_ASSERT_CHANNEL(5, 2 * 10000 + 90);
		SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);

		// This packet sets dpID 2 of type Value to 110
		CMD_ExecuteCommand("uartFakeHex 55AA03070008020200040000006E87", 0);
		Sim_RunFrames(100, false);
		SELFTEST_ASSERT_CHANNEL(1, 110);
		SELFTEST_ASSERT_CHANNEL(5, 2 * 10000 + 110);

		SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);
	}
	int delta = Berry_GetStackSizeTotal() - startStackSize;
	printf("Stack grew %i\n", delta);
	SELFTEST_ASSERT(delta == 0);
 }


void Test_Berry_TuyaMCU_Bytes2() {
	int i;

	// reset whole device
	SIM_ClearOBK(0);

	SIM_UART_InitReceiveRingBuffer(2048);

	CMD_ExecuteCommand("startDriver TuyaMCU", 0);


	// This packet sets dpID 104 of type RAW
	CMD_ExecuteCommand("berry addEventHandler(\"OnDP\", 18, def(value)\n"
		"  f = open('test3.txt', 'w') \n"
		"  f.write(value)\n"
		"  f.close()\n"
		"end)", 0);
	CMD_ExecuteCommand("uartFakeHex 55AA030700101200000C0101003F030100FA040100AA25", 0);
	Sim_RunFrames(100, false);
	byte expected[] = { 0x01, 0x01, 0x00, 0x3F, 0x03, 0x01, 0x00, 0xFA, 0x04, 0x01, 0x00, 0xAA };
	Test_FakeHTTPClientPacket_GET("api/run/test3.txt");
	SELFTEST_ASSERT_HTML_REPLY(expected);
}

void Test_Berry_TuyaMCU_Bytes() {
	int i;

	// reset whole device
	SIM_ClearOBK(0);

	SIM_UART_InitReceiveRingBuffer(2048);

	CMD_ExecuteCommand("startDriver TuyaMCU", 0);

	// per-dpID handler
	CMD_ExecuteCommand("setChannel 1 0", 0);
	// writer test
	CMD_ExecuteCommand("berry addEventHandler(\"OnDP\", 2, def(value)\n"
		"print(value) \n"
		"  f = open('test.txt', 'w') \n"
		"  f.write(value)\n"
		"  f.close()\n"
		"end)", 0);
	// appender test
	CMD_ExecuteCommand("berry addEventHandler(\"OnDP\", 2, def(value)\n"
		"print(value) \n"
		"  f = open('test2.txt', 'a') \n"
		"  f.write(value)\n"
		"  f.close()\n"
		"end)", 0);

	{
		byte bytes[] = { 'A', 'B', 'C' };
		CMD_Berry_RunEventHandlers_IntBytes(CMD_EVENT_ON_DP, 2, bytes, sizeof(bytes));

		// writer test
		Test_FakeHTTPClientPacket_GET("api/run/test.txt");
		SELFTEST_ASSERT_HTML_REPLY("ABC");
		// appender test
		Test_FakeHTTPClientPacket_GET("api/run/test2.txt");
		SELFTEST_ASSERT_HTML_REPLY("ABC");
	}


	{
		byte bytes[] = { 'a', 'd', 'd', ' ', 'a', '!' };
		CMD_Berry_RunEventHandlers_IntBytes(CMD_EVENT_ON_DP, 2, bytes, sizeof(bytes));

		byte other[] = { 'x' };
		// dpID 22 has no handler, will be ignored
		CMD_Berry_RunEventHandlers_IntBytes(CMD_EVENT_ON_DP, 22, other, sizeof(other));


		// writer test
		Test_FakeHTTPClientPacket_GET("api/run/test.txt");
		SELFTEST_ASSERT_HTML_REPLY("add a!");
		// appender test
		Test_FakeHTTPClientPacket_GET("api/run/test2.txt");
		SELFTEST_ASSERT_HTML_REPLY("ABCadd a!");
	}

	{
		byte bytes[] = { 'q', 'Q', 'q', 'Q', 'q', 'Q' };
		CMD_Berry_RunEventHandlers_IntBytes(CMD_EVENT_ON_DP, 2, bytes, sizeof(bytes));

		// writer test
		Test_FakeHTTPClientPacket_GET("api/run/test.txt");
		SELFTEST_ASSERT_HTML_REPLY("qQqQqQ");
		// appender test
		Test_FakeHTTPClientPacket_GET("api/run/test2.txt");
		SELFTEST_ASSERT_HTML_REPLY("ABCadd a!qQqQqQ");
	}

	// appender test
	CMD_ExecuteCommand("berry addEventHandler(\"OnDP\", 3, def(value)\n"
		"  print(value) \n"
		"  f = value[2]\n"
		"  setChannel(5,f)\n"
		"  f = value[3]\n"
		"  setChannel(6,f)\n"
		"  f = (value[2] << 8) + value[3]\n"
		"  setChannel(7,f)\n"
		"end)", 0);

	{
		byte bytes[] = { 0x02, 0x04, 0x08, 0x0A };
		CMD_Berry_RunEventHandlers_IntBytes(CMD_EVENT_ON_DP, 3, bytes, sizeof(bytes));

		SELFTEST_ASSERT_CHANNEL(5, 0x08);
		SELFTEST_ASSERT_CHANNEL(6, 0x0A);
		SELFTEST_ASSERT_CHANNEL(7, (0x08 << 8) + 0x0A);
	}
	CMD_ExecuteCommand("berry addEventHandler(\"OnDP\", 4, def(value)\n"
		"  print(value) \n"
		"  g = value[1]\n"
		"  setChannel(10, g)\n"
		"  h = value[3]\n"
		"  setChannel(11, h)\n"
		"  i = (value[1] * value[3])\n"
		"  setChannel(12, i)\n"
		"end)", 0);

	{
		byte bytes[] = { 0x03, 0x05, 0x07, 0x09 };
		CMD_Berry_RunEventHandlers_IntBytes(CMD_EVENT_ON_DP, 4, bytes, sizeof(bytes));

		SELFTEST_ASSERT_CHANNEL(10, 0x05);
		SELFTEST_ASSERT_CHANNEL(11, 0x09);
		SELFTEST_ASSERT_CHANNEL(12, 0x05 * 0x09);
	}
	CMD_ExecuteCommand("berry addEventHandler(\"OnDP\", 5, def(value)\n"
		"  print(value) \n"
		"  x = value[0]\n"
		"  g = value[1]\n"
		"  h = value[3]\n"
		"  i = (value[1] * value[3])\n"
		"  f = open('test3.txt', 'w') \n"
		"  f.write(str(x))\n"
		"  f.write(' ')\n"
		"  f.write(str(i))\n"
		"  f.close()\n"
		"end)", 0);

	{
		byte bytes[] = { 0x03, 0x05, 0x07, 0x09 };
		CMD_Berry_RunEventHandlers_IntBytes(CMD_EVENT_ON_DP, 5, bytes, sizeof(bytes));

		Test_FakeHTTPClientPacket_GET("api/run/test3.txt");
		SELFTEST_ASSERT_HTML_REPLY("3 45");
	}
}
void Test_Berry_HTTP() {
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	int startStackSize = Berry_GetStackSizeTotal();
	// inject html to state page
	CMD_ExecuteCommand("berry addEventHandler(\"OnHTTP\", \"state\", def(request)\n"
		"	poststr(request,\"MySpecialTestString23432411\") \n"
		"end)", 0);

	Test_FakeHTTPClientPacket_GET("index?state=1");
	SELFTEST_ASSERT_HTML_REPLY_CONTAINS("MySpecialTestString23432411");

	Test_FakeHTTPClientPacket_GET("index?state=1");
	SELFTEST_ASSERT_HTML_REPLY_CONTAINS("MySpecialTestString23432411");

	// custom page
	CMD_ExecuteCommand("berry addEventHandler(\"OnHTTP\", \"mypage123\", def(request)\n"
		"	poststr(request,\"Anything\") \n"
		"end)", 0);
	Test_FakeHTTPClientPacket_GET("mypage123");
	SELFTEST_ASSERT_HTML_REPLY("Anything");
	SELFTEST_ASSERT(Berry_GetStackSizeTotal() == startStackSize);



}
void Test_Berry_MQTTHandler() {
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	CMD_ExecuteCommand("berry addEventHandler(\"OnMQTT\", def(topic, payload)\n"
		"runCmd(\"MQTTHost \"+payload) \n"
		"end)", 0);
	CMD_Berry_RunEventHandlers_Str(CMD_EVENT_ON_MQTT, "xyz", "192.168.0.123");
	SELFTEST_ASSERT_STRING("192.168.0.123", CFG_GetMQTTHost());
	CMD_Berry_RunEventHandlers_Str(CMD_EVENT_ON_MQTT, "xyz", "555.168.0.123");
	SELFTEST_ASSERT_STRING("555.168.0.123", CFG_GetMQTTHost());


}
void Test_Berry_MQTTHandler2() {
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	CMD_ExecuteCommand("berry addEventHandler(\"OnMQTT\", \"myTopic123\", def(payload)\n"
		"runCmd(\"MQTTHost \"+payload+\".extra\") \n"
		"end)", 0);
	CMD_Berry_RunEventHandlers_Str(CMD_EVENT_ON_MQTT, "myTopic123", "444.222");
	SELFTEST_ASSERT_STRING("444.222.extra", CFG_GetMQTTHost());
}

void Test_Berry_HTTP2() {
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	{
		const char *test1 =
			"<!DOCTYPE html>"
			"<html>"
			"<body>"
			"<h1>Hello <?b echo(2+2)?></h1>"
			"</body>"
			"</html>";
		Test_FakeHTTPClientPacket_POST("api/lfs/index.html", test1);
		const char *test1_res =
			"<!DOCTYPE html>"
			"<html>"
			"<body>"
			"<h1>Hello 4</h1>"
			"</body>"
			"</html>";
		Test_FakeHTTPClientPacket_GET("api/run/index.html");
		SELFTEST_ASSERT_HTML_REPLY(test1_res);
	}
	{
		const char *test1 =
			"<!DOCTYPE html>"
			"<html>"
			"<body>"
			"<h1>Hello <?b echo(getVar(\"arg\"))?></h1>"
			"</body>"
			"</html>";
		Test_FakeHTTPClientPacket_POST("api/lfs/indexb.html", test1);
		const char *test1_res =
			"<!DOCTYPE html>"
			"<html>"
			"<body>"
			"<h1>Hello hey</h1>"
			"</body>"
			"</html>";
		Test_FakeHTTPClientPacket_GET("api/run/indexb.html?arg=hey");
		SELFTEST_ASSERT_HTML_REPLY(test1_res);
	}
	{
		const char *test1 =
			"<!DOCTYPE html>\n"
			"<html>\n"
			"<body>\n"
			"<h1>Hello <?b echo(2+2)?></h1>"
			"</body>\n"
			"</html>";
		Test_FakeHTTPClientPacket_POST("api/lfs/index.html", test1);
		const char *test1_res =
			"<!DOCTYPE html>\n"
			"<html>\n"
			"<body>\n"
			"<h1>Hello 4</h1>"
			"</body>\n"
			"</html>";
		Test_FakeHTTPClientPacket_GET("api/run/index.html");
		SELFTEST_ASSERT_HTML_REPLY(test1_res);
	}
	{
		const char *test0 =
			"<!DOCTYPE html>\n"
			"<html>\n"
			"<body>\n"
			"<h1>Hello World</h1>"
			"</body>\n"
			"</html>";
		Test_FakeHTTPClientPacket_POST("api/lfs/index0.html", test0);
		const char *test0_res =
			"<!DOCTYPE html>\n"
			"<html>\n"
			"<body>\n"
			"<h1>Hello World</h1>"
			"</body>\n"
			"</html>";
		Test_FakeHTTPClientPacket_GET("api/run/index0.html");
		SELFTEST_ASSERT_HTML_REPLY(test0_res);
	}
	{
		const char *test2 =
			"<!DOCTYPE html>\n"
			"<html>\n"
			"<body>\n"
			"<ul>\n"
			"<?b for i: 0 .. 2\n"
			"	echo(\"<li>\"+str(i)+\"</li>\")\n"
			"end ?>\n"
			"</ul>"
			"</body>\n"
			"</html>";
		Test_FakeHTTPClientPacket_POST("api/lfs/index2.html", test2);
		const char *test2_res =
			"<!DOCTYPE html>\n"
			"<html>\n"
			"<body>\n"
			"<ul>\n<li>0</li><li>1</li><li>2</li>\n</ul>"
			"</body>\n"
			"</html>";
		Test_FakeHTTPClientPacket_GET("api/run/index2.html");
		SELFTEST_ASSERT_HTML_REPLY(test2_res);
	}
	{
		const char *test3 =
			"<!DOCTYPE html>\n"
			"<html>\n"
			"<body>\n"
			"<ul>\n"
			"<?b for i: 0 .. 2\n"
			"	?><li><?b echo(str(i))?></li>"
			"<?b end ?>\n"
			"</ul>"
			"</body>\n"
			"</html>";
		Test_FakeHTTPClientPacket_POST("api/lfs/index3.html", test3);
		const char *test3_res =
			"<!DOCTYPE html>\n"
			"<html>\n"
			"<body>\n"
			"<ul>\n<li>0</li><li>1</li><li>2</li>\n</ul>"
			"</body>\n"
			"</html>";
		Test_FakeHTTPClientPacket_GET("api/run/index3.html");
		SELFTEST_ASSERT_HTML_REPLY(test3_res);
	}
	{
		const char *test4 =
			"<!DOCTYPE html>\n"
			"<html>\n"
			"<body>\n"
			"<ul>\n"
			"<?b for i: 0 .. 1\n"
			"	?><li>Outer <?b echo(str(i))?><ul>"
			"<?b for j: 0 .. 1\n"
			"	?><li>Inner <?b echo(str(j))?></li>"
			"<?b end ?>\n"
			"</ul></li>"
			"<?b end ?>\n"
			"</ul>\n"
			"</body>\n"
			"</html>";
		Test_FakeHTTPClientPacket_POST("api/lfs/index4.html", test4);
		const char *test4_res =
			"<!DOCTYPE html>\n"
			"<html>\n"
			"<body>\n"
			"<ul>\n"
			"<li>Outer 0<ul><li>Inner 0</li><li>Inner 1</li>\n</ul></li>"
			"<li>Outer 1<ul><li>Inner 0</li><li>Inner 1</li>\n</ul></li>\n"
			"</ul>\n"
			"</body>\n"
			"</html>";
		Test_FakeHTTPClientPacket_GET("api/run/index4.html");
		SELFTEST_ASSERT_HTML_REPLY(test4_res);
	}
	CMD_ExecuteCommand("lfs_format", 0);
	{
		const char *test5 =
			"<!DOCTYPE html>\n"
			"<html>\n"
			"<body>\n"
			"<ul>\n"
			"<?b for i: 0 .. 1\n"
			"	?><li>Outer <?b echo(str(i))?><ul>"
			"<?b for j: 0 .. 4\n"
			"	?><li>Inner <?b echo(str(j))?></li>"
			"<?b end ?>\n"
			"</ul></li>"
			"<?b end ?>\n"
			"</ul>\n"
			"</body>\n"
			"</html>";
		Test_FakeHTTPClientPacket_POST("api/lfs/index5.html", test5);
		const char *test5_res =
			"<!DOCTYPE html>\n"
			"<html>\n"
			"<body>\n"
			"<ul>\n"
			"<li>Outer 0<ul>"
			"<li>Inner 0</li><li>Inner 1</li><li>Inner 2</li><li>Inner 3</li><li>Inner 4</li>\n"
			"</ul></li>"
			"<li>Outer 1<ul>"
			"<li>Inner 0</li><li>Inner 1</li><li>Inner 2</li><li>Inner 3</li><li>Inner 4</li>\n"
			"</ul></li>\n"
			"</ul>\n"
			"</body>\n"
			"</html>";
		Test_FakeHTTPClientPacket_GET("api/run/index5.html");
		SELFTEST_ASSERT_HTML_REPLY(test5_res);
	}

}
void Test_Berry_Button() {
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	CMD_ExecuteCommand("berry addEventHandler(\"OnClick\", 5, def(arg)\n"
		"runCmd(\"addChannel 5 1\") \n"
		"end)", 0);
	CMD_ExecuteCommand("berry addEventHandler(\"OnClick\", 6, def(arg)\n"
		"runCmd(\"addChannel 6 1\") \n"
		"end)", 0);
	SELFTEST_ASSERT_CHANNEL(5, 0);
	SELFTEST_ASSERT_CHANNEL(6, 0);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_PIN_ONCLICK, 5, 0);
	SELFTEST_ASSERT_CHANNEL(5, 1);
	SELFTEST_ASSERT_CHANNEL(6, 0);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_PIN_ONCLICK, 5, 0);
	SELFTEST_ASSERT_CHANNEL(5, 2);
	SELFTEST_ASSERT_CHANNEL(6, 0);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_PIN_ONCLICK, 6, 0);
	SELFTEST_ASSERT_CHANNEL(5, 2);
	SELFTEST_ASSERT_CHANNEL(6, 1);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_PIN_ONCLICK, 7, 0);
	SELFTEST_ASSERT_CHANNEL(5, 2);
	SELFTEST_ASSERT_CHANNEL(6, 1);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_PIN_ONCLICK, 6, 0);
	SELFTEST_ASSERT_CHANNEL(5, 2);
	SELFTEST_ASSERT_CHANNEL(6, 2);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_PIN_ONCLICK, 6, 0);
	SELFTEST_ASSERT_CHANNEL(5, 2);
	SELFTEST_ASSERT_CHANNEL(6, 3);
	CMD_Berry_RunEventHandlers_IntInt(CMD_EVENT_PIN_ONCLICK, 6, 0);
	SELFTEST_ASSERT_CHANNEL(5, 2);
	SELFTEST_ASSERT_CHANNEL(6, 4);


}
void Test_Berry_CmdHandler() {
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	CMD_ExecuteCommand("berry addEventHandler(\"OnCmd\", \"MyCmd\", def(arg)\n"
		"runCmd(\"MQTTHost \"+arg) \n"
		"end)", 0);
	CMD_Berry_RunEventHandlers_Str(CMD_EVENT_ON_CMD, "MyCmd", "192.168.0.123");
	SELFTEST_ASSERT_STRING("192.168.0.123", CFG_GetMQTTHost());

	CMD_ExecuteCommand("MyCmd 555.168.0.123",0);
	SELFTEST_ASSERT_STRING("555.168.0.123", CFG_GetMQTTHost());


}
void Test_Berry_NTP() {
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	NTP_SetSimulatedTime(1654853254);

	SELFTEST_ASSERT_EXPRESSION("$minute", 27);
	SELFTEST_ASSERT_EXPRESSION("$hour", 9);
	SELFTEST_ASSERT_EXPRESSION("$second", 34);
	SELFTEST_ASSERT_EXPRESSION("$day", 5);

	CMD_ExecuteCommand("berry setChannel(5,getVar(\"$minute\"))", 0);
	SELFTEST_ASSERT_CHANNEL(5, 27);

	CMD_ExecuteCommand("berry setChannel(5,getVar(\"$hour\"))", 0);
	SELFTEST_ASSERT_CHANNEL(5, 9);

	CMD_ExecuteCommand("berry setChannel(5,getVar(\"$second\"))", 0);
	SELFTEST_ASSERT_CHANNEL(5, 34);

	CMD_ExecuteCommand("berry setChannel(5,getVar(\"$day\"))", 0);
	SELFTEST_ASSERT_CHANNEL(5, 5);
}


void Test_Berry() {

	Test_Berry_Click_And_Timeout();
	Test_Berry_Click();
	Test_Berry_AutoexecAndBerry();
	Test_Berry_Button();
	Test_Berry_Import_Autorun();
	Test_Berry_HTTP2();
	Test_Berry_HTTP();
	Test_Berry_VarLifeSpan();
    Test_Berry_ChannelSet();
    Test_Berry_CancelThread();
	Test_Berry_SetInterval();
	Test_Berry_SetTimeout();
    Test_Berry_Import();
    Test_Berry_ThreadCleanup();
    Test_Berry_AutoloadModule();
	Test_Berry_StartScriptShortcut();
	Test_Berry_StartScriptShortcut2();
	Test_Berry_PassArg();
	Test_Berry_PassArgFromCommand();
	Test_Berry_PassArgFromCommandWithoutModule();
	Test_Berry_PassArgFromCommandWithModule();
	Test_Berry_CommandRunner();

	Test_Berry_MQTTHandler();
	Test_Berry_MQTTHandler2();
	Test_Berry_CmdHandler();
	Test_Berry_Fibonacci();
	Test_Berry_AddChangeHandler();
	Test_Berry_FileSystem();
	Test_Berry_TuyaMCU();
	Test_Berry_TuyaMCU_Bytes();
	Test_Berry_TuyaMCU_Bytes2();
	Test_Berry_NTP();
}

#endif
