# Tasmota Device Groups - ACK Behavior Analysis

## Overview
Tasmota implements a sophisticated ACK (acknowledgment) system to ensure reliable delivery of device group messages across UDP multicast. This document details the exact behavior for cross-compatibility with OpenBK.

---

## ACK Transmission Behavior (How Tasmota SENDS ACKs)

### When Tasmota Sends ACKs
Tasmota sends an ACK whenever it **receives** a message from another device, with the following exceptions:

```c
// From support_device_groups.ino, line 236-249
if (flags == DGR_FLAG_ACK) {
    // This is already an ACK message - don't ACK it
    goto write_log;
}

if (flags == DGR_FLAG_ANNOUNCEMENT) {
    // This is just a heartbeat - don't ACK it
    goto write_log;
}

// If this is a received message, send an ack message to the sender.
if (device_group_member) {
    if (received) {
        if (!(flags & DGR_FLAG_MORE_TO_COME)) {  // Don't ACK if MORE_TO_COME flag is set
            *(message_ptr - 2) = DGR_FLAG_ACK;
            *(message_ptr - 1) = 0;
            SendReceiveDeviceGroupMessage(device_group, device_group_member, message, message_ptr - message, false);
        }
    }
}
```

### ACK Message Format
- **Header**: Same as normal messages (TASMOTA_DGR + group_name)
- **Sequence**: The sequence number being ACK'd (extracted from the received message)
- **Flags**: Set to `DGR_FLAG_ACK` (value 8)
- **Payload**: Empty (just the header + sequence + ACK flag)
- **Delivery**: Unicast directly to the sender's IP address (not multicast)

### Key ACK Transmission Rules
1. **No ACK for ACKs**: If a message is already an ACK, don't send another ACK (prevents infinite loops)
2. **No ACK for Announcements**: Announcements don't require ACKs
3. **No ACK if MORE_TO_COME**: If the `DGR_FLAG_MORE_TO_COME` flag is set, don't send ACK yet (wait for the complete message)
4. **Direct Unicast**: ACKs are sent directly back to the sender, not multicast

---

## ACK Reception Behavior (How Tasmota RECEIVES/VALIDATES ACKs)

### Tracking Received ACKs
When Tasmota receives an ACK message:

```c
// From support_device_groups.ino, line 236-242
if (flags == DGR_FLAG_ACK) {
    if (received && device_group_member && 
        (message_sequence > device_group_member->acked_sequence || 
         device_group_member->acked_sequence - message_sequence < 64536)) {
        device_group_member->acked_sequence = message_sequence;
    }
    goto write_log;
}
```

### ACK Validation Logic
- **Accept newer sequences**: If `received_sequence > acked_sequence`, accept the ACK
- **Wrap-around handling**: If `acked_sequence - message_sequence < 64536`, this accounts for sequence number wrap-around (uint16_t)
- **Store acked_sequence**: Update the member's `acked_sequence` field with the sequence that was ACK'd

### Member Structure Tracking
```c
struct device_group_member {
  struct device_group_member * flink;
  IPAddress ip_address;
  uint16_t received_sequence;      // Last sequence received from this member
  uint16_t acked_sequence;          // Last sequence this member has ACK'd
  uint32_t unicast_count;           // Count of direct unicast resends
};
```

**Important**: 
- `received_sequence` = last message sequence received FROM this member
- `acked_sequence` = last message sequence FROM US that this member has acknowledged

---

## Resend Logic (How Tasmota Handles Unacknowledged Messages)

### When Resends Occur
Tasmota runs an ACK check periodically (~200ms initially, exponential backoff):

```c
// From support_device_groups.ino, line 927-994
if (device_group->next_ack_check_time) {
    if ((long)(now - device_group->next_ack_check_time) >= 0) {
        
        // After initial discovery, iterate through members
        struct device_group_member * device_group_member;
        while ((device_group_member = *flink)) {
            
            // If member hasn't ACK'd the latest message...
            if (device_group_member->acked_sequence != device_group->outgoing_sequence) {
                
                // Check member timeout (45 seconds)
                if ((long)(now - device_group->member_timeout_time) >= 0) {
                    // Remove member from group
                    *flink = device_group_member->flink;
                    free(device_group_member);
                    continue;
                }
                
                // Otherwise, unicast directly to this member
                SendReceiveDeviceGroupMessage(device_group, device_group_member, 
                                             device_group->message, 
                                             device_group->message_length, false);
                device_group_member->unicast_count++;
                acked = false;
            }
            flink = &device_group_member->flink;
        }
        
        // If all members ACK'd, clear ack check time
        if (acked) {
            device_group->next_ack_check_time = 0;
            device_group->message_length = 0;
        }
        // Otherwise, use exponential backoff for next check
        else {
            device_group->ack_check_interval *= 2;
            if (device_group->ack_check_interval > 5000) 
                device_group->ack_check_interval = 5000;
            device_group->next_ack_check_time = now + device_group->ack_check_interval;
        }
    }
}
```

### Resend Strategy
- **Exponential Backoff**: Start at 200ms, double each check (200→400→800→1600→3200→5000ms)
- **Direct Unicast**: Resends are sent directly to the member, not multicast
- **Incremental Counter**: Track `unicast_count` for debugging
- **Member Removal**: Remove any member that hasn't been heard from for 45 seconds (DGR_MEMBER_TIMEOUT)

---

## Member Discovery and Announcements

### Initial Discovery (Startup)
When a device joins a group:

```c
// From support_device_groups.ino, line 828-836
device_group->initial_status_requests_remaining = 10;
device_group->ack_check_interval = 200;
device_group->next_ack_check_time = now + device_group->ack_check_interval;
```

Tasmota sends 10 `STATUS_REQUEST` messages (flag=2) at 200ms intervals to discover other members.

### Heartbeat Announcements
Every 60 seconds plus random jitter (0-10 seconds):

```c
// From support_device_groups.ino, line 1005-1010
if ((long)(now - device_group->next_announcement_time) >= 0) {
    SendReceiveDeviceGroupMessage(device_group, nullptr, 
                                  device_group->message, 
                                  BeginDeviceGroupMessage(device_group, DGR_FLAG_ANNOUNCEMENT, true) 
                                  - device_group->message, false);
    device_group->next_announcement_time = now + DGR_ANNOUNCEMENT_INTERVAL + random(10000);
}
```

**Purpose**: Announce presence to keep members aware without sending data updates.

---

## DeviceGroupStatus JSON Output

When queried, Tasmota outputs member ACK status:

```c
// From support_device_groups.ino, line 892
snprintf_P(buffer, sizeof(buffer), 
    PSTR("%s,{\"IPAddress\":\"%s\",\"ResendCount\":%u,\"LastRcvdSeq\":%u,\"LastAckedSeq\":%u}"), 
    buffer, IPAddressToString(device_group_member->ip_address), 
    device_group_member->unicast_count, 
    device_group_member->received_sequence, 
    device_group_member->acked_sequence);
```

Example output:
```json
{
  "DeviceGroup1": [
    {
      "IPAddress": "192.168.1.100",
      "ResendCount": 2,
      "LastRcvdSeq": 1047,
      "LastAckedSeq": 1045
    }
  ]
}
```

**Interpretation**:
- `LastRcvdSeq` = Last sequence number received FROM this device
- `LastAckedSeq` = Last sequence number FROM US that this device has acknowledged
- `ResendCount` = Number of times we've directly unicast to this device (indicates reliability issues)

---

## Critical Compatibility Requirements for OpenBK

### 1. **ACK Format Requirements**
✅ OpenBK correctly sends ACKs with:
- Correct sequence number from received message
- `DGR_FLAG_ACK` (value 8)
- Unicast delivery (not multicast)

### 2. **ACK Reception Requirements**
✅ OpenBK must track `acked_sequence` per member:
- Update whenever an ACK is received
- Compare against `outgoing_sequence` to detect unacknowledged messages

### 3. **Sequence Number Tracking**
✅ OpenBK must maintain:
- `received_sequence` = last seq received FROM members
- `acked_sequence` = last seq FROM US that members ACK'd
- Proper 16-bit overflow handling

### 4. **Message Handling**
⚠️ **IMPORTANT**: OpenBK must handle the `DGR_FLAG_MORE_TO_COME` flag:
- **DO NOT** send ACK if this flag is present in received message
- Wait for complete multi-part message before sending ACK

### 5. **No Infinite ACK Loops**
✅ OpenBK correctly avoids:
- Sending ACK to an ACK message
- Not ACKing announcements

---

## Timing Constants (from Tasmota)

```c
#define DGR_MEMBER_TIMEOUT        45000    // Remove member if no message for 45 seconds
#define DGR_ANNOUNCEMENT_INTERVAL 60000    // Send announcement every 60 seconds + jitter
```

Member discovery interval: **200ms** (starts here, exponential backoff to 5000ms max)

---

## Summary: The Complete ACK Cycle

### Scenario: Tasmota sends a command to OpenBK

1. **Tasmota sends message** (multicast or unicast)
   - Flag = 0 (normal data update)
   - Sequence = X (incremented)
   - outgoing_sequence = X

2. **OpenBK receives message**
   - Validates sequence
   - Processes data
   - **MUST send ACK back**

3. **OpenBK sends ACK** (unicast directly to Tasmota)
   - Flag = DGR_FLAG_ACK (8)
   - Sequence = X (same as received)

4. **Tasmota receives ACK**
   - Validates: acked_sequence < sequence < acked_sequence + 65536
   - Updates: device_group_member->acked_sequence = X

5. **Tasmota ACK check** (every 200-5000ms)
   - Compares: acked_sequence (X) == outgoing_sequence (X)?
   - **YES**: Message successfully delivered, clear ack check
   - **NO**: Unicast resend directly to OpenBK, increase interval

6. **If OpenBK doesn't respond for 45 seconds**
   - Tasmota removes OpenBK from member list

---

## Implementation Validation Checklist

For OpenBK device groups to work correctly with Tasmota:

- [ ] OpenBK sends ACK message when receiving data messages (excluding MORE_TO_COME)
- [ ] OpenBK ACK contains correct sequence number from received message
- [ ] OpenBK ACK uses DGR_FLAG_ACK (value 8)
- [ ] OpenBK sends ACKs via unicast to sender's IP
- [ ] OpenBK tracks received_sequence per member
- [ ] OpenBK tracks acked_sequence per member
- [ ] OpenBK handles sequence wrap-around (16-bit)
- [ ] OpenBK doesn't send ACK to ACK messages
- [ ] OpenBK doesn't send ACK to announcements
- [ ] OpenBK respects DGR_FLAG_MORE_TO_COME (don't ACK yet)
- [ ] OpenBK sends heartbeat announcements every ~60 seconds
- [ ] OpenBK sends initial discovery (10 status requests at startup)
- [ ] Tasmota can see OpenBK in DeviceGroupStatus with correct sequence tracking
