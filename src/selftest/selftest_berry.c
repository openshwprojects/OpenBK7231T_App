#ifdef WINDOWS

#include "selftest_local.h"

void Test_Berry_ChannelSet() {
    int i;

    // reset whole device
    SIM_ClearOBK(0);

    // Make sure channels start at 0
    CMD_ExecuteCommand("setChannel 1 0", 0);
    CMD_ExecuteCommand("setChannel 2 0", 0);
    
    SELFTEST_ASSERT_CHANNEL(1, 0);
    SELFTEST_ASSERT_CHANNEL(2, 0);

    // Run Berry code that sets channels
    CMD_ExecuteCommand("berry channelSet(1, 42)", 0);
    CMD_ExecuteCommand("berry channelSet(2, 123)", 0);
    
    // Verify the channels were set correctly
    SELFTEST_ASSERT_CHANNEL(1, 42);
    SELFTEST_ASSERT_CHANNEL(2, 123);
    
    // Test setting multiple channels in one script
    CMD_ExecuteCommand("berry channelSet(1, 99); channelSet(2, 88)", 0);
    
    // Verify the updated values
    SELFTEST_ASSERT_CHANNEL(1, 99);
    SELFTEST_ASSERT_CHANNEL(2, 88);
}

void Test_Berry_CancelThread() {
    int i;

    // reset whole device
    SIM_ClearOBK(0);

    // Make sure channels start at 0
    CMD_ExecuteCommand("setChannel 1 0", 0);
    CMD_ExecuteCommand("setChannel 2 0", 0);
    
    SELFTEST_ASSERT_CHANNEL(1, 0);
    SELFTEST_ASSERT_CHANNEL(2, 0);

    // Run Berry code that creates a delayed thread that would set channels
    // and stores the thread ID in a global variable
    CMD_ExecuteCommand("berry thread_id = scriptDelayMs(100, def() channelSet(1, 42); channelSet(2, 123); end); print(thread_id)", 0);
    
    // Now cancel the thread using the thread_id
    CMD_ExecuteCommand("berry cancel(thread_id)", 0);
    
    // Run scheduler long enough for the original delay to have completed if it wasn't cancelled
    for (i = 0; i < 20; i++) {
        SVM_RunThreads(10);
    }
    
    // Verify the channels were NOT set (they should still be 0)
    SELFTEST_ASSERT_CHANNEL(1, 0);
    SELFTEST_ASSERT_CHANNEL(2, 0);
    
    // Now let's test the positive case - create a thread and let it complete
    CMD_ExecuteCommand("berry scriptDelayMs(50, def() channelSet(1, 99); channelSet(2, 88); end)", 0);
    
    // Run scheduler to let the thread complete
    for (i = 0; i < 20; i++) {
        SVM_RunThreads(10);
    }
    
    // Verify the channels WERE set this time
    SELFTEST_ASSERT_CHANNEL(1, 99);
    SELFTEST_ASSERT_CHANNEL(2, 88);
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
        "  channelSet(1, 42)\n"
        "  channelSet(2, 84)\n"
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
    CMD_ExecuteCommand("berry import autoexec; channelSet(3, autoexec.TEST_VALUE)", 0);
    
    // Verify the constant from the module was correctly accessed
    SELFTEST_ASSERT_CHANNEL(3, 123);
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
    CMD_ExecuteCommand("berry scriptDelayMs(10, def() channelSet(1, 42); end)", 0);
    
    // Run the scheduler to let the Berry script complete
    for (i = 0; i < 5; i++) {
        SVM_RunThreads(10);
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
		SVM_RunThreads(20);
    
    // Channel 2 should be set already (before the delay)
    SELFTEST_ASSERT_CHANNEL(2, 123);
    
    // Channel 3 should not be set yet (after the delay)
    SELFTEST_ASSERT_CHANNEL(3, 0);
    
    // Now let the script complete
    for (i = 0; i < 10; i++) {
        SVM_RunThreads(10);
    }
    
    // Now channel 3 should be set
    SELFTEST_ASSERT_CHANNEL(3, 456);
    
    // Run one more Berry script to confirm both types still work
    CMD_ExecuteCommand("berry channelSet(1, 789)", 0);
    SELFTEST_ASSERT_CHANNEL(1, 789);
}
void Test_Berry_AutoloadModule() {
    // reset whole device
    SIM_ClearOBK(0);
		CMD_ExecuteCommand("lfs_format", 0);

    // Create a Berry module file
    Test_FakeHTTPClientPacket_POST("api/lfs/autoexec.be",
        "autoexec = module('autoexec')\n"
        "\n"
        "autoexec.init = def (self)\n"
        "  print('Hello from autoexec.be')\n"
        "  channelSet(5, 42) # Set a channel so we can verify init ran\n"
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
    
    // Run scheduler to let any scripts complete
    for (int i = 0; i < 5; i++) {
        SVM_RunThreads(10);
    }
    
    // Verify that the Berry module was loaded and initialized (it should have set channel 5 to 42)
    SELFTEST_ASSERT_CHANNEL(5, 42);
    
    // Test that we can now use the module functions directly
    CMD_ExecuteCommand("berry import autoexec; autoexec.init(); channelSet(6, 99)", 0);
    SELFTEST_ASSERT_CHANNEL(6, 99);
}

void Test_Berry_StartScriptShortcut() {
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	// Create a Berry module file
	Test_FakeHTTPClientPacket_POST("api/lfs/test.be",
		"def mySample()\n"
		"  print('Hello from test.be')\n"
		"  channelSet(5, 2025) # Set a channel so we can verify init ran\n"
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
		SVM_RunThreads(10);
	}

	// Verify that the Berry module was loaded and initialized (it should have set channel 5 to 2025)
	SELFTEST_ASSERT_CHANNEL(5, 2025);
}


void Test_Berry_PassArg() {
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	// Create a Berry module file
	Test_FakeHTTPClientPacket_POST("api/lfs/test.be",
		"def mySample(x)\n"
		"  print('Hello from test.be')\n"
		"  channelSet(5, x) # Set a channel so we can verify init ran\n"
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
		SVM_RunThreads(10);
	}

	// Verify that the Berry module was loaded and initialized (it should have set channel 5 to 15)
	SELFTEST_ASSERT_CHANNEL(5, 15);
}



void Test_Berry_PassArgFromCommand() {
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	// Make sure channel starts at 0
	CMD_ExecuteCommand("setChannel 5 0", 0);
	SELFTEST_ASSERT_CHANNEL(5, 0);

	// Simulate a device restart by running the autoexec.txt script
	CMD_ExecuteCommand("berry def mySample(x) channelAdd(5, x) end", 0);
	CMD_ExecuteCommand("berry mySample(15)", 0);
	SELFTEST_ASSERT_CHANNEL(5, 15);
	CMD_ExecuteCommand("berry mySample(15)", 0);
	SELFTEST_ASSERT_CHANNEL(5, 30);
	CMD_ExecuteCommand("berry mySample(2)", 0);
	SELFTEST_ASSERT_CHANNEL(5, 32);
	CMD_ExecuteCommand("berry mySample(-5)", 0);
	SELFTEST_ASSERT_CHANNEL(5, 27);
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
		"  channelAdd(5, x) # Set a channel so we can verify init ran\n"
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

	// Create a Berry module file
	Test_FakeHTTPClientPacket_POST("api/lfs/test.be",
		"test = module('test')\n"
		"\n"
		"# Add functions to the module\n"
		"test.mySample = def(x)\n"
		"  channelAdd(5, x) # Set a channel so we can verify init ran\n"
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

void Test_Berry_MQTTHandler() {
	/*
	// reset whole device
	SIM_ClearOBK(0);
	CMD_ExecuteCommand("lfs_format", 0);

	// Create a Berry module file
	Test_FakeHTTPClientPacket_POST("api/lfs/test.be",
		"test = module('test')\n"
		"\n"
		"# Add functions to the module\n"
		"test.myDataHandler = def(topic, payload)\n"
		"  printf(\"Got data \" + topic + \" and \" + payload)\n"
		"end\n"
		"test.init = def (self)\n"
		"  addBerryHandler(\"onMQTT\",\"test\",\"myDataHandler\")\n"
		"  return self\n"
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
	*/
}
void Test_Berry() {
    Test_Berry_ChannelSet();
    Test_Berry_CancelThread();
    Test_Berry_Import();
    Test_Berry_ThreadCleanup();
    Test_Berry_AutoloadModule();
	Test_Berry_StartScriptShortcut();
	Test_Berry_PassArg();
	Test_Berry_PassArgFromCommand();
	Test_Berry_PassArgFromCommandWithoutModule();
	Test_Berry_PassArgFromCommandWithModule();
	Test_Berry_CommandRunner();

	Test_Berry_MQTTHandler();
}

#endif
