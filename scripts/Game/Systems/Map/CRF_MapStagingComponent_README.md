# CRF Map Staging Component

A dynamic boundary staging system for Arma Reforger that allows progressive map control through timed or manual boundary activation, deactivation, and deletion.

## Overview

The Map Staging Component provides a simple API for mission makers to create dynamic play areas that change over time. Instead of static boundaries, you can create sequences that activate new areas, remove old ones, or delete boundaries entirely as the mission progresses.

## Quick Setup

1. **Add the Component**: Add `CRF_MapStagingComponent` to your game mode
2. **Place Boundaries**: Create GameBoundary entities in your world and name them clearly, or else they wont be identified correctly on init
3. **Add the Component**: Set up the polylines for your zones, be sure to set BOTH  the GameBoundry/CRF_Polyzone & CRF_PolyzoneTrigger (child object in heirarchy) 's Reversed toggles to off. Does NOT work with REVERSED.
3. **Configure Stages**: Set up your boundary stages in the component inspector.
4. **Test**: Enable debug logging and test your sequence

## Stage Types

- **ACTIVATION**: Boundary starts hidden, appears when stage executes
- **DEACTIVATION**: Boundary starts visible, gets hidden when stage executes  
- **DELETION**: Boundary gets permanently deleted when stage executes

## Basic Configuration

### Base Settings
- **Initial Delay**: Time after safestart ends before auto-staging begins (default: 30s)
- **Auto-Start**: Whether staging starts automatically or requires manual triggers
- **Start/End Sounds**: Audio feedback for stage transitions
- **Debug Logging**: Enable for troubleshooting and testing
- **Final Stage Messages**: Message + Sub-Message for when all stages are completely (blank is valid)

### Per-Stage Settings
- **Boundary Entity Name**: Exact name of your GameBoundary entity
- **Stage Type**: ACTIVATION, DEACTIVATION, or DELETION. For stage timers, it enacts at the END of the timer.
- **Duration**: Timer length in seconds
- **Display Text**: Message shown under timer during countdown
- **Completion Messages**: Custom popup when stage finishes (blank is valid)
- **Visual Colors**: Boundary colors for ACTIVE and ACTIVATED states. ACTIVE is only present if using STAGE TIMERS. ACTIVATED is the color state in how it will be displayed if its ACTIVATIONTYPE is set to deactivation/activation. Deletion just makes it disappear. (Opacity change for NO fill is valid. IE: Just wanted the polygon fill color to disappear for deactivation type stage.)

## API Reference

### Get Component Instance
```csharp
CRF_MapStagingComponent staging = CRF_MapStagingComponent.Cast(
    GetGame().GetGameMode().FindComponent(CRF_MapStagingComponent)
);
```

### Main Methods

**Execute Single Stage**
```csharp
staging.ExecuteStaging(stageIndex, useTimer, chainToNext);
// stageIndex: Which stage (0, 1, 2...)
// useTimer: true = countdown timer, false = immediate
// chainToNext: true = continue to next stages, false = stop after this one
```

**Execute Full Sequence**
```csharp
staging.ExecuteStagingSequence(startIndex);
// startIndex: Which stage to begin sequence from
```

**Utility Methods**
```csharp
// Manual start (bypasses safestart)
staging.BeginStaging();

// Emergency stop
staging.StopStaging();
```

## Example Scenarios

### Two-Stage Objective Progression (Destructor Chained)
**Use Case**: Progressive objective unlocking - Rush-like with timers between as a 'R&R' period

**Base Configuration**:
- Initial Delay: 0 (not used with manual trigger)
- Auto-Start: false (manual script control only)
- Debug Logging: true (for testing)
- Stage Start Sound: Radio beep
- Stage End Sound: Success chime

**Stage 0 Settings** (First Objective Unlocked):
- Boundary Entity Name: "FirstObjectiveProtection"
- Stage Type: DELETION (removes protection around first objective)
- Duration: 180 (3 minute refractory period before zone unlock)
- Display Text: "First objective protection ending..."
- Main Completion Message: "First Objective Active"
- Sub Completion Message: "Eliminate the first target"
- Completion Duration: 8 seconds
- Active Colors: Yellow fill (0.8 0.8 0.2 0.5), Orange border (0.8 0.4 0 1)
- Activated Colors: Not used (boundary deleted)

**Stage 1 Settings** (Second Objective Unlocked):
- Boundary Entity Name: "SecondObjectiveBarrier"
- Stage Type: ACTIVATION (reveals second objective area)
- Duration: 240 (4 minute refractory period before activation)
- Display Text: "Second objective area opening..."
- Main Completion Message: "Final Objective Active"
- Sub Completion Message: "Move to the final target zone"
- Completion Duration: 10 seconds
- Active Colors: Orange fill (0.8 0.4 0 0.5), Dark border (0.1 0.1 0.1 1)
- Activated Colors: Green fill (0.2 0.6 0.2 0.4), Dark border (0.1 0.1 0.1 1)

**Script Usage**: 
```csharp
// Single MCOMS Example (triggers chained sequence) 
// (Can add normal Rush checks for objects present for multi/sister MCOMS like we already do)
CRF_MapStagingComponent staging = CRF_MapStagingComponent.Cast(
    GetGame().GetGameMode().FindComponent(CRF_MapStagingComponent)
);
staging.ExecuteStaging(0, true, true); // With timer, chain to next stage
```

### Timed Map Expansion via Zone Deletion
**Use Case**: Force players out of spawn/safe areas after a timer period

**Base Configuration**:
- Initial Delay: 120 (2 minutes after safestart)
- Auto-Start: false (script triggered when needed)
- Debug Logging: false (production ready)
- Stage Start Sound: Warning alarm
- Stage End Sound: Danger alert

**Stage 0 Settings**:
- Boundary Entity Name: "SpawnProtectionZone"
- Stage Type: DELETION (permanently removes boundary)
- Duration: 300 (5 minute warning timer)
- Display Text: "Spawn protection ending in..."
- Main Completion Message: "Spawn Protection Removed"
- Sub Completion Message: "Safe zone no longer exists"
- Completion Duration: 10 seconds
- Active Colors: Red fill (0.8 0.2 0.2 0.6), Yellow border (0.8 0.8 0.2 1)
- Activated Colors: Not used (boundary deleted)

**Script Usage**: `staging.ExecuteStaging(0, true, false);` // With timer, no chain

### Multi-Stage Auto Timed Restriction Sequence
**Use Case**: Progressive map shrinking over time to force engagement

**Base Configuration**:
- Initial Delay: 180 (3 minutes after safestart)
- Auto-Start: true (fully automatic sequence)
- Debug Logging: false
- Stage Start Sound: Map update beep
- Stage End Sound: Boundary warning
- Final Completion Message: "Final Combat Zone Active"
- Final Sub Message: "Fight for control of the remaining area"
- Final Duration: 15 seconds

**Stage 0 Settings** (Outer Ring Removal):
- Boundary Entity Name: "OuterRingBoundary"
- Stage Type: DEACTIVATION (moves boundary away)
- Duration: 600 (10 minute timer)
- Display Text: "Outer area closing in..."
- Main Completion Message: "Outer Zone Closed"
- Sub Completion Message: "Move to inner areas"
- Active Colors: Yellow fill (0.8 0.8 0.2 0.5), Orange border (0.8 0.4 0 1)
- Activated Colors: Red fill (0.6 0.1 0.1 0.3), Dark red border (0.4 0.1 0.1 1)

**Stage 1 Settings** (Middle Ring Removal):
- Boundary Entity Name: "MiddleRingBoundary"
- Stage Type: DEACTIVATION
- Duration: 480 (8 minute timer)
- Display Text: "Combat zone shrinking..."
- Main Completion Message: "Middle Zone Closed"
- Sub Completion Message: "Final zone activation incoming"
- Active Colors: Orange fill (0.8 0.4 0 0.6), Red border (0.6 0.2 0.2 1)
- Activated Colors: Dark red fill (0.4 0.1 0.1 0.4), Black border (0.1 0.1 0.1 1)

**Stage 2 Settings** (Final Area Activation):
- Boundary Entity Name: "FinalCombatZone"
- Stage Type: ACTIVATION (reveals final objective area)
- Duration: 60 (1 minute final warning)
- Display Text: "Final zone activating..."
- Main Completion Message: "Final Combat Zone Active"
- Sub Completion Message: "Secure the final objective"
- Active Colors: Bright orange (1.0 0.5 0 0.7), Bright border (1.0 0.8 0 1)
- Activated Colors: Bright green (0.3 0.8 0.3 0.6), White border (0.9 0.9 0.9 1)

**Script Usage**: Automatic (no script needed) or `staging.ExecuteStagingSequence(0);` for manual start

## Usage Examples

### Destructor Script Integration
```csharp
// When object is destroyed, trigger staging with timer
CRF_MapStagingComponent staging = CRF_MapStagingComponent.Cast(
    GetGame().GetGameMode().FindComponent(CRF_MapStagingComponent)
);
staging.ExecuteStaging(0, true, false);
```

### Conditional Staging
```csharp
// Different stages based on conditions
if (playerCount > 10) {
    // Immediate execution, chain to next stages
    staging.ExecuteStaging(1, false, true);
} else {
    // Timer execution, single stage only
    staging.ExecuteStaging(2, true, false);
}
```

### Emergency Controls
```csharp
// Admin commands or fail-safes
if (emergencyCondition) {
    staging.StopStaging(); // Halt all staging
}

// Skip to final stage
staging.ExecuteStaging(lastStageIndex, false, false);
```

## Best Practices

### IMPORTANT Setup Tips
- Use clear, descriptive names for boundary entities
- Position boundaries at their final locations in the editor
- Test with debug logging enabled
- Configure faction keys in CRF_PolyZone components
- Only use non-reversed GameBoundary/CRF_PolyZone entities

### Performance
- The system uses entity caching for better performance
- Boundary states are replicated for late-joining players
- RPC system ensures cross-environment compatibility

### Troubleshooting
- Enable debug logging to see detailed execution flow
- Check entity names match exactly (case-sensitive)
- Verify boundaries have CRF_PolyZone components
- Test in both Workbench and dedicated server environments

## Technical Notes

- **Replication**: All critical state is replicated for multiplayer
- **JIP Support**: Late-joining players receive current boundary states
- **Cross-Environment**: Works in both Workbench testing and dedicated servers
- **Performance**: Uses caching and optimization for large boundary counts
- **Safety**: Server-only execution prevents client-side manipulation

## Integration

The component integrates with:
- CRF Safestart Manager (for auto-timing)
- CRF Gamemode system (for state management)
- Standard Arma Reforger GameBoundary entities
- CRF_PolyZone visual system

## Version Information

- **Author**: Trist
- **Created**: August 2025
- **Component**: CRF_MapStagingComponent
- **Dependencies**: CRF Framework, GameBoundary entities, CRF_PolyZone
