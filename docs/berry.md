# Berry

## What

Berry is a lightweight scripting language designed for small embedded systems. In OpenBeken, Berry provides a more powerful alternative to the built-in scripting system, with features like:

- Object-oriented programming
- First-class functions and closures
- Proper exception handling
- Module system for code organization
- Rich standard library

## Why

While OpenBeken's built-in scripting is great for simple automation tasks, Berry offers several advantages:

- More structured programming with proper functions and classes
- Better error handling with try/catch
- More powerful data structures (maps, lists, etc.)
- Ability to create reusable modules
- Familiar syntax for those coming from languages like Python or JavaScript

## How to Enable Berry

Berry is currently enabled by default on some platforms (like WINDOWS), but not on all platforms. 

To enable Berry on other platforms:

1. Edit `src/obk_config.h` 
2. Find your platform's section (e.g., `#elif PLATFORM_W600`)
3. Add the following line within your platform's section:
   ```c
   #define ENABLE_OBK_BERRY 1
   ```
4. Recompile and flash your firmware

If you're unsure which platform you're using, check the existing entries in `obk_config.h` and look for the section that matches your device.

## Basic Usage

To run Berry code, use the `berry` command followed by your code:

```
berry print("Hello from Berry!")
```

You can also create Berry script files with the `.be` extension and load them:

```
berry import mymodule
```

## Caveats and Limitations

**Note: Berry support in OpenBeken is currently experimental.** The API and functionality may change in future releases without notice.

- The Berry API is not finalized and may change between firmware versions
- When using Berry alongside traditional OBK scripts, be aware that Berry uses thread IDs starting at 5000
- Using thread IDs above 5000 in traditional OBK scripts may cause unexpected behavior or conflicts with Berry threads
- Berry consumes more RAM than traditional scripts, so complex scripts may lead to memory issues on devices with limited RAM
- Error handling is still being improved, so some errors may not provide helpful messages

## Key Functions

- `setChannel(channel, value)` - Set a channel value
- `getChannel(channel)` - Get a channel value
- `scriptDelayMs(ms, function)` - Run a function after a delay, returns a thread ID
- `addChangeHandler(event, relation, value, function)` - Run a function when an event occurs, returns a thread ID
- `cancel(threadId)` - Cancel a delayed function or event handler by its thread ID

## Examples

### Toggle a relay every second

```berry
def toggle_relay()
  current = getChannel(1)
  setChannel(1, 1 - current)
  scriptDelayMs(1000, toggle_relay)
end

toggle_relay()
```

### React to events

```berry
# Store the handler ID so we can cancel it later if needed
handler_id = addChangeHandler("Channel3", "=", 1, def()
  print("Button pressed!")
  setChannel(1, 1)  # Turn on relay
end)

# Later, to cancel the handler:
# cancel(handler_id)
```

### Cancelling timers and event handlers

```berry
# Create a repeating timer that toggles a relay every second
def setup_toggle()
  def toggle_relay()
    current = getChannel(1)
    setChannel(1, 1 - current)
    return scriptDelayMs(1000, toggle_relay)  # Return the new timer ID
  end
  
  return toggle_relay()  # Start the timer and return its ID
end

# Set up a button handler that cancels the timer when pressed
def setup_cancel_button()
  return addChangeHandler("Channel3", "=", 1, def()
    print("Cancelling the toggle timer")
    cancel(toggle_timer_id)
    # We could also cancel ourselves with: cancel(button_handler_id)
  end)
end

# Start everything
toggle_timer_id = setup_toggle()
button_handler_id = setup_cancel_button()

# To cancel everything later:
# cancel(toggle_timer_id)
# cancel(button_handler_id)
```

### Create a module

Create a file named `mymodule.be`:

```berry
mymodule = module("mymodule")

mymodule.init = def(self)
  print("Module initialized")
  return self
end

mymodule.toggle_channel = def(self, channel)
  current = getChannel(channel)
  setChannel(channel, 1 - current)
end

return mymodule
```

Then use it:

```berry
import mymodule
mymodule.toggle_channel(1)
```
