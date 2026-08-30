---
title: StudioEvents
type: note
permalink: metahooksv/studio-events
---

# StudioEvents Plugin Documentation

## Overview

StudioEvents is a MetaHook plugin for controlling and filtering Studio model event sounds in Half-Life. It implements an intelligent anti-spam mechanism that prevents excessive sound-event playback and improves the gameplay experience.

## Main Features

### 1. Sound Event Filtering
- Intercepts and processes Studio model events (event 5004)
- Decides whether to play, delay, or block sounds according to configured rules
- Supports distinct filtering policies for "identical sounds" and "different sounds"

### 2. Anti-Spam Mechanism
- **Identical-sound filtering**: Prevents duplicate playback of the same sound file for the same entity and frame
- **Different-sound filtering**: Controls the minimum interval between different sound events
- **Automatic cleanup**: Periodically removes expired sound-event records to optimize memory usage

### 3. Delayed Playback System
- Optional delayed sound playback
- When the anti-spam mechanism blocks a sound, it can be deferred until playback is allowed
- Automatically processes the delayed queue and triggers sounds at the appropriate time

### 4. Player Sound Blocking
- Can selectively block all sound events from player entities
- Useful when player-originated sounds need to be reduced

## Configuration Variables (CVARs)

### cl_studiosnd_anti_spam_diff
- **Default**: 0.5
- **Type**: FCVAR_CLIENTDLL | FCVAR_ARCHIVE
- **Description**: Minimum interval between different sound events (seconds)
- **Purpose**: Prevents too many different sounds from playing in a short period

### cl_studiosnd_anti_spam_same
- **Default**: 1.0
- **Type**: FCVAR_CLIENTDLL | FCVAR_ARCHIVE
- **Description**: Minimum interval between identical sound events (seconds)
- **Purpose**: Prevents repeated playback of the same sound

### cl_studiosnd_anti_spam_delay
- **Default**: 0
- **Type**: FCVAR_CLIENTDLL | FCVAR_ARCHIVE
- **Description**: Whether delayed playback is enabled (0 = block immediately; nonzero = delay playback)
- **Purpose**: Controls whether blocked sounds are discarded or played later

### cl_studiosnd_block_player
- **Default**: 0
- **Type**: FCVAR_CLIENTDLL | FCVAR_ARCHIVE
- **Description**: Whether to block sound events from player entities (0 = do not block; >0 = block)
- **Purpose**: Completely suppresses player-related sound events

### cl_studiosnd_debug
- **Default**: 0
- **Type**: FCVAR_CLIENTDLL
- **Description**: Debug-mode switch (0 = disabled; >0 = enabled)
- **Purpose**: Prints sound-event processing information to the console

## Core Data Structures

### studio_event_sound_t
```cpp
struct studio_event_sound_s {
    char name[64];      // Sound file name
    int entindex;       // Entity index
    int frame;          // Animation frame
    float time;         // Timestamp
}
```

Stores key sound-event information to determine whether a sound is a duplicate and when it may be played again.

### Global Containers
- **g_StudioEventSoundPlayed**: Stores records of played sound events
- **g_StudioEventSoundDelayed**: Stores sound events awaiting delayed playback

## Core Functions

### HUD_Init()
- Plugin initialization entry point
- Registers all CVAR configuration variables
- Clears the sound-event record list
- Calls the original HUD_Init

### HUD_VidInit()
- Called when the video mode is initialized
- Clears all sound-event records (played and delayed)
- Ensures state is reset on map changes or reconnections

### HUD_Frame(double a1)
- Called once per frame
- Processes the delayed playback queue
- Checks whether delayed sounds have reached their playback time
- Validates entity validity (using `messagenum`)
- Calls HUD_StudioEvent to play delayed sounds whose time has arrived

### HUD_StudioEvent(const mstudioevent_s* ev, const cl_entity_s* ent)
Core filtering function. Its processing flow is as follows:

1. **Event-type check**: Processes only event 5004 (sound event)
2. **Player-blocking check**: Blocks immediately when `cl_studiosnd_block_player` is enabled and the entity is a player
3. **History traversal**:
   - Removes expired sound records
   - Checks for conflicts with recently played sounds
   - Distinguishes the decision logic for "identical sounds" and "different sounds"
4. **Decision handling**:
   - If a conflict is found and delay is enabled: add the sound to the delayed queue
   - If a conflict is found but delay is disabled: block it immediately
   - If there is no conflict: play and record it immediately
5. **Original-function call**: Ultimately calls `gExportfuncs.HUD_StudioEvent`

## How It Works

### Anti-Spam Decision Logic

```
For every new sound event:
├─ Is it a player and is blocking enabled?
│  └─ Yes → Block
├─ Traverse history:
│  ├─ Has the record expired?
│  │  └─ Yes → Delete the record
│  ├─ Is it the same sound (entity, frame, and name all match)?
│  │  └─ Yes → Check whether it is within the anti_spam_same interval
│  │     └─ Yes → Mark a conflict and record the maximum wait time
│  └─ Is it a different sound?
│     └─ Check whether it is within the anti_spam_diff interval
│        └─ Yes → Mark a conflict and record the maximum wait time
└─ Was a conflict found?
   ├─ Yes, and delay enabled → Add to the delayed queue
   ├─ Yes, and delay disabled → Block immediately
   └─ No → Play and record immediately
```

### Delayed Playback Mechanism

1. Blocked sounds are added to `g_StudioEventSoundDelayed` with a calculated playback time
2. The delayed queue is checked in `HUD_Frame` every frame
3. When the time arrives, an `mstudioevent_s` structure is constructed and `HUD_StudioEvent` is called
4. Verifies that the entity is still valid (`messagenum` matches)
5. Removes the sound from the delayed queue after playback

## Usage Scenarios

### Scenario 1: Preventing Overlapping Sounds
```
Setting: cl_studiosnd_anti_spam_same 1.0
Effect: The same sound can be played again only after at least 1 second
```

### Scenario 2: Controlling Sound Density
```
Setting: cl_studiosnd_anti_spam_diff 0.5
Effect: There is an interval of at least 0.5 seconds between any sound events
```

### Scenario 3: Delay Rather Than Discard
```
Setting: cl_studiosnd_anti_spam_delay 1
Effect: Blocked sounds are played automatically when permitted
```

### Scenario 4: Debug Mode
```
Setting: cl_studiosnd_debug 1
Effect: The console prints the processing status of every sound event
     [StudioEvents] Played xxx.wav
     [StudioEvents] Blocked xxx.wav
     [StudioEvents] Delayed xxx.wav
```

## Technical Notes

### 1. Entity Validity Validation
Uses the `messagenum` field to determine whether an entity remains valid, preventing playback of sounds from deleted entities.

### 2. Memory Management
- Uses `std::vector` to manage sound records dynamically
- Automatically removes expired records to prevent memory leaks
- Clears all records completely on map changes

### 3. Time Management
- Uses `gEngfuncs.GetClientTime()` to obtain client time
- All time comparisons use floating-point values, which provide sufficient precision

### 4. String Handling
- Uses `strcpy` to copy sound names (a fixed 64-byte buffer)
- Uses `strcmp` to compare whether sounds are identical

## Potential Improvements

1. **TODO comment**: The code suggests that `cl_parsecount` may be needed instead of `messagenum` to determine entity validity
2. **Buffer safety**: Consider using `strncpy` instead of `strcpy` to improve safety
3. **Configuration range checks**: CVARs could be given boundary-value validation
4. **Performance optimization**: A hash table could optimize lookups when there are many sound events
