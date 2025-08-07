/****************************************************************************************
 * File Name:        CRF_MapStagingComponent
 * Author:           Trist
 * Date Created:     08/03/25
 * Description:      CRF Boundry staging system with options for timed stages and manual firing via outside script
 * Comments:        -  Trist: Please feel free edit, use, modify, expand, whatever the fuck you can think of im inexperienced as shit
 ***************************************************************************************/

/*
 * EXTERNAL API REFERENCE - Available functions for outside scripts:
 * 
 * CRF_MapStagingComponent staging = CRF_MapStagingComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_MapStagingComponent));
 * 
 * staging.TriggerStage(int stageIndex)           - Trigger a specific stage with timer (if enabled) or immediately (if disabled)
 * staging.TriggerStageChainless(int stageIndex)  - Trigger a single stage immediately without chaining to next stages
 * staging.BeginStaging()                         - Start the full staging sequence from stage 0
 * staging.StopStaging()                          - Emergency stop all staging activity and clear timers
 * 
 * Note: All stage indices are 0-based (first stage = 0, second stage = 1, etc.)

	This shits heavily commented because I tried my best to code it then have copilot iterate on it
	Then I tried to make it work with the CRFPolyzoneTrigger and it was a fucking nightmare <3

 */

enum CRF_BoundaryStageType
{
	ACTIVATION = 0,
	DELETION = 1,
	DEACTIVATION = 2
}

enum CRF_BoundaryStageState
{
	INACTIVE = 0,	// Not yet triggered/chained
	ACTIVE = 1,		// Started, during timer
	ACTIVATED = 2	// Finished timer or completed
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sBoundaryEntityName"}, "%1")]
class CRF_BoundaryStageData
{
	[Attribute("", UIWidgets.EditBox, "Name of the GameBoundary entity")]
	string m_sBoundaryEntityName;
	
	[Attribute("0", UIWidgets.ComboBox, "Stage behavior type", "", ParamEnumArray.FromEnum(CRF_BoundaryStageType))]
	CRF_BoundaryStageType m_eStageType;
	
	[Attribute("120", UIWidgets.EditBox, "Duration of this stage in seconds")]
	int m_iStageDuration;
	
	[Attribute("", UIWidgets.EditBox, "Text to display during this stage")]
	string m_sStageDisplayText;
	
	[Attribute("Stage Complete", UIWidgets.EditBox, "Main completion message for this stage (leave empty for default)", category: "Completion Message")]
	string m_sStageCompletionMainMessage;
	
	[Attribute("Boundary updated", UIWidgets.EditBox, "Sub completion message for this stage (leave empty for default)", category: "Completion Message")]
	string m_sStageCompletionSubMessage;
	
	[Attribute("10", UIWidgets.EditBox, "Duration (seconds) to display stage completion message", category: "Completion Message")]
	int m_iStageCompletionDuration;
	
	// Visual state colors for boundary zones
	[Attribute("0.8 0.3 0 0.45", UIWidgets.ColorPicker, "ACTIVE state - Polygon fill color (timer running)")]
	ref Color m_cActivePolygonColor;
	
	[Attribute("0.045 0.045 0.045 1", UIWidgets.ColorPicker, "ACTIVE state - Polygon border color")]
	ref Color m_cActivePolygonBorderColor;
	
	[Attribute("0.5 0.1 0.1 0.45", UIWidgets.ColorPicker, "ACTIVATED state - Polygon fill color (completed)")]
	ref Color m_cActivatedPolygonColor;
	
	[Attribute("0.045 0.045 0.045 1", UIWidgets.ColorPicker, "ACTIVATED state - Polygon border color")]
	ref Color m_cActivatedPolygonBorderColor;
}

[ComponentEditorProps(category: "Game Mode Component", description: "Map Staging System")]
class CRF_MapStagingComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_MapStagingComponent : SCR_BaseGameModeComponent
{
	[Attribute("30", UIWidgets.EditBox, "Initial delay after safestart ends (seconds)", category: "Base Configuration")]
	int m_iInitialDelay;
	
	[Attribute("true", UIWidgets.CheckBox, "Use automatic timer staging (if false, stages only trigger via scripting)", category: "Base Configuration")]
	bool m_bUseAutomaticTimers;
	
	// Global audio settings for all stages
	[Attribute("{E23715DAF7FE2E8A}Sounds/Items/Equipment/Radios/Samples/Items_Radio_Turn_On.wav", UIWidgets.ResourcePickerThumbnail, "Audio to play when any stage timer starts", "wav", category: "Base Configuration")]
	ResourceName m_sStageStartSound;
	
	[Attribute("{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav", UIWidgets.ResourcePickerThumbnail, "Audio to play when any stage completes", "wav", category: "Base Configuration")]
	ResourceName m_sStageEndSound;
	
	[Attribute("false", UIWidgets.CheckBox, "Enable debug logging", category: "Base Configuration")]
	bool m_bDebugEnabled;
	
	[Attribute("All Stages Complete", UIWidgets.EditBox, "Main message shown when all stages are completed", category: "Base Configuration")]
	string m_sFinalCompletionMainMessage;
	
	[Attribute("Final boundaries active", UIWidgets.EditBox, "Sub message shown when all stages are completed", category: "Base Configuration")]
	string m_sFinalCompletionSubMessage;
	
	[Attribute("15", UIWidgets.EditBox, "Duration (seconds) to display final completion message", category: "Base Configuration")]
	int m_iFinalCompletionMessageDuration;
	
	[Attribute("", UIWidgets.Auto, "List of boundary stages", category: "Stage Configuration")]
	ref array<ref CRF_BoundaryStageData> m_aBoundaryStages;
	
 	[Attribute("STAGING DOCUMENTATION\n\n- Place & rename Gameboundry prefabs (be sure to name them, can even be the same name.)\n- Position them where you want the final boundary areas\n- Enable debug if problems arise\n\n=== STAGE TYPES ===\nACTIVATION/DEACTIVATION: Boundary either is removed or placed at the END of the timer/manual trigger. For now only usable with the NON-reversed CRFPolyzoneTrigger as it will just render across the whole map elsewise.\nDELETION: Boundary exists at normal position, gets deleted when timer completes/is called\n\n\n=== Details ===\n- Automatic Timer Enabled: Stages execute in sequence after safestart ends. If Disabling automatic timers, control via external scripts\n- Initial Delay: Starts on safestart end, starts first stage when done\n\n\n\n=== EXTERNAL SCRIPT INTEGRATION ===\n\nreminder: Index #'s start at 0\n// Get staging system reference\nCRF_MapStagingComponent staging = CRF_MapStagingComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_MapStagingComponent));\n\n// Trigger specific stage with timer (will go on to next)\nstaging.TriggerStage(**STAGE INDEX #**);\n\n// Trigger stage immediately without chaining to next stage\nstaging.TriggerStageChainless(**STAGE INDEX #**);\n\n// Start full staging sequence\nstaging.BeginStaging();\n\n// Emergency stop all staging\nstaging.StopStaging();\n\n\n=== USE CASES ===\n• Timed based zone activations/deletions\n• Objective-based area unlocking\n• Call/Trigger based activations\n• Mission phase area restrictions\n• Adding regroup period to staged gamemodes", UIWidgets.EditBoxMultiline, "Setup Instructions & Script Examples", category: "Documentation")]
	string m_sInstructions;
	
	// Public state for display
	[RplProp()]
	bool countDownActive = false;
	
	[RplProp(onRplName: "ShowMessage")]
	string m_sMessageContent = "";
	
	[RplProp()]
	string m_sTimerText = ""; // Dedicated property for timer display
	
	[RplProp(onRplName: "PlaySound", condition: RplCondition.NoOwner)]
	string m_sSoundToPlay = "";
	
	[RplProp()]
	string m_sStageText = "";
	
	// Internal states for jippers
	[RplProp()]
	protected int m_iCurrentStage = 0;
	
	[RplProp()]
	int m_iCurrentTimer = 0; // Made public
	
	[RplProp()]
	protected bool m_bStagingActive = false;
	protected ref map<string, vector> m_mOriginalPositions;
	protected ref map<string, CRF_BoundaryStageState> m_mStageStates; // Track state of each boundary
	protected string m_sStoredMessageContent;
	protected SCR_PopUpNotification m_PopUpNotification;
	
	// rpc for jippers
	[RplProp()]
	protected ref array<string> m_aReplicatedBoundaryNames = {};
	
	[RplProp(onRplName: "OnBoundaryStatesChanged")]
	protected ref array<int> m_aReplicatedBoundaryStates = {};
	
	// Performance safeguards
	protected int m_iInitRetryCount = 0;
	protected static const int MAX_INIT_RETRIES = 3;
	
	// Performance~ cache stage data lookups
	protected ref map<string, ref CRF_BoundaryStageData> m_mStageDataCache;
	
	// Performance: cache entity references to avoid repeated FindEntityByName calls
	protected ref map<string, IEntity> m_mBoundaryEntityCache;
	
	//------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Initialize cache maps on ALL clients and server (needed for RPC calls)
		m_mOriginalPositions = new map<string, vector>();
		m_mStageStates = new map<string, CRF_BoundaryStageState>();
		m_mStageDataCache = new map<string, ref CRF_BoundaryStageData>();
		m_mBoundaryEntityCache = new map<string, IEntity>();
		if (!m_aBoundaryStages)
			m_aBoundaryStages = new array<ref CRF_BoundaryStageData>();
		
		// Initialize replicated arrays
		m_aReplicatedBoundaryNames = new array<string>();
		m_aReplicatedBoundaryStates = new array<int>();
		
		// Server-only initialization
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		// Delay initialization to ensure world is fully loaded
		GetGame().GetCallqueue().CallLater(InitializeBoundaries, 3000, false);
		// Start monitoring safestart (single call, not repeating)
		GetGame().GetCallqueue().CallLater(MonitorSafestart, 5000, false);
	}
	
	//-----------------------------------------------------------------------------------------------
	void InitializeBoundaries()
	{
		// Server-only operation for multiplayer safety
		if (RplSession.Mode() == RplMode.Client)
			return;
			
		if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] Initializing boundaries...");
		
		if (!m_aBoundaryStages || m_aBoundaryStages.Count() == 0)
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] WARNING: No boundary stages configured!");
			return;
		}
		
		bool foundMissingEntity = false;
		
		for (int i = 0; i < m_aBoundaryStages.Count(); i++)
		{
			CRF_BoundaryStageData stageData = m_aBoundaryStages[i];
			if (!stageData || !stageData.m_sBoundaryEntityName || stageData.m_sBoundaryEntityName.IsEmpty())
			{
				if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] WARNING: Invalid stage data at index %1", i));
				continue;
			}
			
			// Try to find boundary entity (use cached lookup for performance)
			IEntity boundaryEntity = GetCachedBoundaryEntity(stageData.m_sBoundaryEntityName);
			if (!boundaryEntity)
			{
				Print(string.Format("[CRF_MapStagingComponent] ERROR: Boundary '%1' not found! Check entity name in world.", stageData.m_sBoundaryEntityName));
				foundMissingEntity = true;
				continue; // Continue processing other entities instead of immediate return
			}
			
			vector originalPos = boundaryEntity.GetOrigin();
			m_mOriginalPositions.Set(stageData.m_sBoundaryEntityName, originalPos);
			
			// Initialize stage state as INACTIVE but don't set initial colors
			m_mStageStates.Set(stageData.m_sBoundaryEntityName, CRF_BoundaryStageState.INACTIVE);
			
			// Add to replicated arrays for late-joining players
			m_aReplicatedBoundaryNames.Insert(stageData.m_sBoundaryEntityName);
			m_aReplicatedBoundaryStates.Insert(CRF_BoundaryStageState.INACTIVE);
			
			// Cache stage data for performance optimization
			m_mStageDataCache.Set(stageData.m_sBoundaryEntityName, stageData);
			
			if (stageData.m_eStageType == CRF_BoundaryStageType.ACTIVATION)
			{
				vector newPos = Vector(originalPos[0] + 10000, originalPos[1] + 1000, originalPos[2] + 10000);
				boundaryEntity.SetOrigin(newPos);
				if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] ACTIVATION boundary '%1' moved away and higher", stageData.m_sBoundaryEntityName));
			}
			else if (stageData.m_eStageType == CRF_BoundaryStageType.DEACTIVATION)
			{
				// DEACTIVATION boundaries start at their original position
				if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] DEACTIVATION boundary '%1' ready at original position", stageData.m_sBoundaryEntityName));
			}
			else if (m_bDebugEnabled)
			{
				Print(string.Format("[CRF_MapStagingComponent] DELETION boundary '%1' ready for removal", stageData.m_sBoundaryEntityName));
			}
		}
		
		// Handle missing entities with retry logic (performance safeguard)
		if (foundMissingEntity && m_iInitRetryCount < MAX_INIT_RETRIES)
		{
			m_iInitRetryCount++;
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Retrying boundary initialization in 5 seconds (attempt %1/%2)", m_iInitRetryCount, MAX_INIT_RETRIES));
			GetGame().GetCallqueue().CallLater(InitializeBoundaries, 5000, false);
			return;
		}
		else if (foundMissingEntity)
		{
			Print("[CRF_MapStagingComponent] ERROR: Maximum initialization retries exceeded! Some boundaries will not function properly.");
		}
		
		// Replicate the initial boundary states to all clients
		Replication.BumpMe();
		
		int successfullyInitialized = m_aReplicatedBoundaryNames.Count();
		if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Successfully initialized %1/%2 boundary stages", successfullyInitialized, m_aBoundaryStages.Count()));
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Fast cached entity lookup to avoid expensive FindEntityByName calls
	 * @param entityName - Name of the entity to find
	 * @return IEntity - The found entity or null
	 */
	IEntity GetCachedBoundaryEntity(string entityName)
	{
		// Null/empty check
		if (!entityName || entityName.IsEmpty())
			return null;
		
		// Keep cache initialized
		if (!m_mBoundaryEntityCache)
		{
			m_mBoundaryEntityCache = new map<string, IEntity>();
		}
		
		// Check cache first
		IEntity cachedEntity;
		if (m_mBoundaryEntityCache.Find(entityName, cachedEntity))
		{
			// Verify entity still exists
			if (cachedEntity)
				return cachedEntity;
			else
			{
				// Clean up invalid cache entry
				m_mBoundaryEntityCache.Remove(entityName);
			}
		}
		
		// big dick scary get when getting cached entities fails, me practicing optimization lul
		IEntity entity = GetGame().GetWorld().FindEntityByName(entityName);
		if (entity)
		{
			m_mBoundaryEntityCache.Set(entityName, entity);
		}
		
		return entity;
	}
	
	//------------------------------------------------------------------------------------------------
	void MonitorSafestart()
	{
		// Server-only operation for multiplayer safety
		if (RplSession.Mode() == RplMode.Client)
			return;
			
		// No auto staging if auto timer is disabled
		if (!m_bUseAutomaticTimers)
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] Automatic timer staging disabled - waiting for manual script trigger");
			GetGame().GetCallqueue().Remove(MonitorSafestart);
			return;
		}
		
		// Check safestart status and game state, stole harrys snippet
		CRF_SafestartManager safestart = CRF_SafestartManager.GetInstance();
		if (!safestart || safestart.GetSafestartStatus() || CRF_Gamemode.GetInstance().m_GamemodeState != CRF_EGamemodeState.GAME)
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] Waiting for safestart to end and game to start...");
			GetGame().GetCallqueue().CallLater(MonitorSafestart, 10000, false);
			return;
		}
		
		// Safestart has ended and game is live - start staging
		if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Game is live, starting staging in %1 seconds", m_iInitialDelay));
		GetGame().GetCallqueue().CallLater(StartStaging, m_iInitialDelay * 1000, false);
		m_bStagingActive = true;
		Replication.BumpMe(); // Replicate staging activation
	}
	
	//------------------------------------------------------------------------------------------------
	void StartStaging()
	{
		// rpl to prevent client exec
		if (RplSession.Mode() == RplMode.Client)
			return;
			
		if (!m_bStagingActive || !m_aBoundaryStages || m_iCurrentStage >= m_aBoundaryStages.Count())
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] StartStaging blocked - no valid stage");
			return;
		}
		
		CRF_BoundaryStageData currentStage = m_aBoundaryStages[m_iCurrentStage];
		if (!currentStage)
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] ERROR: Stage %1 data is null!", m_iCurrentStage + 1));
			return;
		}
		
		m_iCurrentTimer = currentStage.m_iStageDuration;
		countDownActive = true;
		m_sStageText = currentStage.m_sStageDisplayText; // Set stage text for HUD display
		
		// Update boundary visual state to ACTIVE (timer running)
		UpdateBoundaryVisualState(currentStage.m_sBoundaryEntityName, CRF_BoundaryStageState.ACTIVE);
		
		// Play stage start sound if configured
		if (!m_sStageStartSound.IsEmpty())
		{
			m_sSoundToPlay = m_sStageStartSound;
			Replication.BumpMe();
		}
		
		if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Starting stage %1: '%2' (%3s)", 
			m_iCurrentStage + 1, m_sStageText, m_iCurrentTimer));
		
		Replication.BumpMe();
		GetGame().GetCallqueue().CallLater(UpdateStageTimer, 1000, true);
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateStageTimer()
	{
		// Server-only operation for multiplayer safety
		if (RplSession.Mode() == RplMode.Client)
			return;
			
		if (!countDownActive)
		{
			GetGame().GetCallqueue().Remove(UpdateStageTimer);
			return;
		}
		
		m_sTimerText = SCR_FormatHelper.FormatTime(m_iCurrentTimer);
		Replication.BumpMe();
		
		if (m_bDebugEnabled && (m_iCurrentTimer % 10 == 0 || m_iCurrentTimer <= 10))
			Print(string.Format("[CRF_MapStagingComponent] Stage %1 timer: %2s remaining", m_iCurrentStage + 1, m_iCurrentTimer));
		
		if (m_iCurrentTimer <= 0)
		{
			GetGame().GetCallqueue().Remove(UpdateStageTimer);
			countDownActive = false;
			m_sTimerText = ""; // Clear timer text when stage completes
			Replication.BumpMe();
			ExecuteStage();
			return;
		}
		
		m_iCurrentTimer--;
	}
	
	//------------------------------------------------------------------------------------------------
	void ExecuteStage()
	{
		// Server-only operation for mp
		if (RplSession.Mode() == RplMode.Client)
			return;
			
		if (!m_aBoundaryStages || m_iCurrentStage >= m_aBoundaryStages.Count())
			return;
		
		CRF_BoundaryStageData currentStage = m_aBoundaryStages[m_iCurrentStage];
		if (!currentStage)
			return;
		
		IEntity boundaryEntity = GetCachedBoundaryEntity(currentStage.m_sBoundaryEntityName);
		string stageTypeString;
		
		if (boundaryEntity)
		{
			if (currentStage.m_eStageType == CRF_BoundaryStageType.ACTIVATION)
			{
				vector originalPos;
				if (m_mOriginalPositions.Find(currentStage.m_sBoundaryEntityName, originalPos))
				{
					boundaryEntity.SetOrigin(originalPos);
					stageTypeString = "ACTIVATED";
					if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] ACTIVATION boundary '%1' activated", currentStage.m_sBoundaryEntityName));
				}
			}
			else if (currentStage.m_eStageType == CRF_BoundaryStageType.DEACTIVATION)
			{
				vector originalPos;
				if (m_mOriginalPositions.Find(currentStage.m_sBoundaryEntityName, originalPos))
				{
					vector newPos = Vector(originalPos[0] + 10000, originalPos[1] + 1000, originalPos[2] + 10000);
					boundaryEntity.SetOrigin(newPos);
					stageTypeString = "DEACTIVATED";
					if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] DEACTIVATION boundary '%1' moved away and deactivated", currentStage.m_sBoundaryEntityName));
				}
			}
			else
			{
				SCR_EntityHelper.DeleteEntityAndChildren(boundaryEntity);
				stageTypeString = "DELETED";
				if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] DELETION boundary '%1' removed", currentStage.m_sBoundaryEntityName));
			}
			
			// Update boundary visual state to ACTIVATED 
			UpdateBoundaryVisualState(currentStage.m_sBoundaryEntityName, CRF_BoundaryStageState.ACTIVATED);
			
			// Play stage end sound if configured
			if (!m_sStageEndSound.IsEmpty())
			{
				m_sSoundToPlay = m_sStageEndSound;
				Replication.BumpMe();
			}
		}
		
		// Show completion popup with custom message
		string completionMessage;
		if (!currentStage.m_sStageCompletionMainMessage.IsEmpty())
		{
			// Use custom stage completion message with full format support
			string subMessage;
			if (currentStage.m_sStageCompletionSubMessage.IsEmpty())
				subMessage = string.Format("Play area %1", stageTypeString);
			else
				subMessage = currentStage.m_sStageCompletionSubMessage;
				
			completionMessage = string.Format("%1|%2|%3", 
				currentStage.m_sStageCompletionMainMessage, 
				currentStage.m_iStageCompletionDuration, 
				subMessage);
		}
		else
		{
			// Use default format
			completionMessage = string.Format("Stage %1 Complete|10|Play area %2", m_iCurrentStage + 1, stageTypeString);
		}
		
		m_sMessageContent = completionMessage;
		Replication.BumpMe();
		ShowMessage();
		
		// Move to next stage or finish
		m_iCurrentStage++;
		Replication.BumpMe(); // Replicate stage progression
		if (m_aBoundaryStages && m_iCurrentStage < m_aBoundaryStages.Count())
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Moving to stage %1 in 2s", m_iCurrentStage + 1));
			GetGame().GetCallqueue().CallLater(StartStaging, 2000, false);
		}
		else
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] All stages completed!");
			m_bStagingActive = false;
			m_sStageText = ""; // Clear stage text when finished
			m_sTimerText = ""; // Clear timer text when finished
			
			// Use customizable final completion message
			m_sMessageContent = string.Format("%1|%2|%3", m_sFinalCompletionMainMessage, m_iFinalCompletionMessageDuration, m_sFinalCompletionSubMessage);
			Replication.BumpMe();
			ShowMessage();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void ShowMessage()
	{
		if (m_sMessageContent == m_sStoredMessageContent)
			return;
		
		m_PopUpNotification = SCR_PopUpNotification.GetInstance();
		m_sStoredMessageContent = m_sMessageContent;
		
		array<string> messageSplitArray = {};
		m_sMessageContent.Split("|", messageSplitArray, false);
		
		if (messageSplitArray.Count() != 3)
			return;

		string mainMessage = messageSplitArray[0];
		string time = messageSplitArray[1];
		string subMessage = messageSplitArray[2];
		
		if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Popup: '%1' (%2s) - '%3'", mainMessage, time, subMessage));
		m_PopUpNotification.PopupMsg(mainMessage, time.ToFloat(), subMessage);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Play sound on all clients - called automatically when m_sSoundToPlay changes
	 */
	void PlaySound()
	{
		if (m_sSoundToPlay.IsEmpty())
			return;
		
		if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Playing sound: %1", m_sSoundToPlay));
		
		// Play the sound locally
		AudioSystem.PlaySound(m_sSoundToPlay);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Update the visual state (colors) of a boundary based on its current stage state
	 * @param boundaryName - Name of the boundary entity to update
	 * @param newState - The new visual state to apply
	 */
	void UpdateBoundaryVisualState(string boundaryName, CRF_BoundaryStageState newState)
	{
		// Call both local and RPC version - I THINK THIS IS OKAY?
		RPC_UpdateBoundaryVisualState(boundaryName, newState);
		Rpc(RPC_UpdateBoundaryVisualState, boundaryName, newState);
	}
	
	/**
	 * RPC method to update boundary colors on all clients
	 * @param boundaryName - Name of the boundary entity to update
	 * @param newState - The new visual state to apply
	 */
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_UpdateBoundaryVisualState(string boundaryName, CRF_BoundaryStageState newState)
	{
		// This runs on ALL clients to update their polygon widgets - I THINK THIS IS OKAY?
		
		// Safety checks
		if (!boundaryName || boundaryName.IsEmpty())
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] RPC_UpdateBoundaryVisualState: Invalid boundary name");
			return;
		}
		
		// Find the boundary entity (use cached lookup for performance)
		IEntity boundaryEntity = GetCachedBoundaryEntity(boundaryName);
		if (!boundaryEntity)
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] UpdateBoundaryVisualState: Boundary '%1' not found", boundaryName));
			return;
		}
		
		// Find the corresponding stage data for color information (use cache for performance)
		CRF_BoundaryStageData stageData = null;
		if (m_mStageDataCache)
		{
			m_mStageDataCache.Find(boundaryName, stageData);
		}
		
		// Fallback to linear search if cache missed or not initialized
		if (!stageData && m_aBoundaryStages)
		{
			for (int i = 0; i < m_aBoundaryStages.Count(); i++)
			{
				if (m_aBoundaryStages[i] && m_aBoundaryStages[i].m_sBoundaryEntityName == boundaryName)
				{
					stageData = m_aBoundaryStages[i];
					break;
				}
			}
		}
		
		if (!stageData)
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] UpdateBoundaryVisualState: Stage data not found for boundary '%1'", boundaryName));
			return;
		}
		
		// Get the CRF_PolyZone component from the boundary entity
		CRF_PolyZone polyZone = CRF_PolyZone.Cast(boundaryEntity.FindComponent(CRF_PolyZone));
		if (!polyZone)
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] UpdateBoundaryVisualState: CRF_PolyZone component not found on '%1'", boundaryName));
			return;
		}
		
		// Update colors based on the new state (skip INACTIVE state)
		Color fillColor, borderColor;
		string stateString;
		
		switch (newState)
		{
			case CRF_BoundaryStageState.INACTIVE:
				// No color changes for INACTIVE state
				if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Boundary '%1' visual state updated to: INACTIVE (no color change)", boundaryName));
				return;
				
			case CRF_BoundaryStageState.ACTIVE:
				fillColor = stageData.m_cActivePolygonColor;
				borderColor = stageData.m_cActivePolygonBorderColor;
				stateString = "ACTIVE";
				break;
				
			case CRF_BoundaryStageState.ACTIVATED:
				fillColor = stageData.m_cActivatedPolygonColor;
				borderColor = stageData.m_cActivatedPolygonBorderColor;
				stateString = "ACTIVATED";
				break;
		}
		
		// Apply the colors to the poly zone
		if (fillColor)
		{
			polyZone.m_cPolygonColor = fillColor;
		}
		if (borderColor)
		{
			polyZone.m_cPolygonBorderColor = borderColor;
		}
		
		// Update existing widgets if active
		if (polyZone.m_MapEntity && polyZone.m_wCanvasWidget)
		{
			MapConfiguration mapConfig = polyZone.m_MapEntity.GetMapConfig();
			if (mapConfig)
			{
				// Real-time update: delete and recreate widget with new colors
				polyZone.DeleteMapWidget(mapConfig);
				polyZone.CreateMapWidget(mapConfig);
				
				if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Widget updated in real-time for '%1' with new colors", boundaryName));
			}
		}
		else if (m_bDebugEnabled)
		{
			Print(string.Format("[CRF_MapStagingComponent] Colors applied to '%1' - widget will update when map is opened", boundaryName));
		}
		
		
		
		// Update the stored state - only server
		if (Replication.IsServer())
		{
			m_mStageStates.Set(boundaryName, newState);
			
			// Update replicated arrays for late-joining players
			int boundaryIndex = m_aReplicatedBoundaryNames.Find(boundaryName);
			if (boundaryIndex != -1)
			{
				m_aReplicatedBoundaryStates[boundaryIndex] = newState;
				Replication.BumpMe(); // Notify replication system of change
			}
		}
		
		if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Boundary '%1' visual state updated to: %2", boundaryName, stateString));
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * JIP handling
	 */
	void OnBoundaryStatesChanged()
	{
		// Skip on server - server already has the correct states
		if (Replication.IsServer())
			return;
		
		// Safety checks for replicated arrays
		if (!m_aReplicatedBoundaryNames || !m_aReplicatedBoundaryStates)
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] OnBoundaryStatesChanged: Replicated arrays not initialized");
			return;
		}
		
		if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] Applying replicated boundary states for late-joining client");
		
		// Apply all replicated boundary states
		for (int i = 0; i < m_aReplicatedBoundaryNames.Count() && i < m_aReplicatedBoundaryStates.Count(); i++)
		{
			string boundaryName = m_aReplicatedBoundaryNames[i];
			CRF_BoundaryStageState state = m_aReplicatedBoundaryStates[i];
			
			// Skip empty/null boundary names
			if (!boundaryName || boundaryName.IsEmpty())
				continue;
			
			// Update the visual state without sending RPC (we're receiving one)
			RPC_UpdateBoundaryVisualState(boundaryName, state);
		}
	}
	
	//-------------------------------------------------------------------------------------------------
	/**
	 * Helper method to execute a single stage's boundary action and play completion sound
	 * @param stageData - The stage data to execute
	 * @param stageIndex - Index of the stage (for completion message)
	 * @param isChainedExecution - Whether this is part of a sequence (affects completion message)
	 * @return string - The stage type string for completion message
	 */
	string ExecuteStageBoundaryAction(CRF_BoundaryStageData stageData, int stageIndex, bool isChainedExecution = false)
	{
		if (!stageData)
			return "";
		
		IEntity boundaryEntity = GetCachedBoundaryEntity(stageData.m_sBoundaryEntityName);
		if (!boundaryEntity)
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] ExecuteStageBoundaryAction failed - boundary entity '%1' not found", stageData.m_sBoundaryEntityName));
			return "";
		}
		
		string stageTypeString;
		
		// Execute the boundary action
		if (stageData.m_eStageType == CRF_BoundaryStageType.ACTIVATION)
		{
			vector originalPos;
			if (m_mOriginalPositions.Find(stageData.m_sBoundaryEntityName, originalPos))
			{
				boundaryEntity.SetOrigin(originalPos);
				stageTypeString = "ACTIVATED";
				string logPrefix;
				if (isChainedExecution)
					logPrefix = "";
				else
					logPrefix = "Single execution: ";
				if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] %1ACTIVATION boundary '%2' activated", logPrefix, stageData.m_sBoundaryEntityName));
			}
		}
		else if (stageData.m_eStageType == CRF_BoundaryStageType.DEACTIVATION)
		{
			vector originalPos;
			if (m_mOriginalPositions.Find(stageData.m_sBoundaryEntityName, originalPos))
			{
				vector newPos = Vector(originalPos[0] + 10000, originalPos[1] + 1000, originalPos[2] + 10000);
				boundaryEntity.SetOrigin(newPos);
				stageTypeString = "DEACTIVATED";
				string logPrefix;
				if (isChainedExecution)
					logPrefix = "";
				else
					logPrefix = "Single execution: ";
				if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] %1DEACTIVATION boundary '%2' moved away and deactivated", logPrefix, stageData.m_sBoundaryEntityName));
			}
		}
		else
		{
			SCR_EntityHelper.DeleteEntityAndChildren(boundaryEntity);
			stageTypeString = "DELETED";
			string logPrefix;
			if (isChainedExecution)
				logPrefix = "";
			else
				logPrefix = "Single execution: ";
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] %1DELETION boundary '%2' removed", logPrefix, stageData.m_sBoundaryEntityName));
		}
		
		// Update boundary visual state to ACTIVATED
		UpdateBoundaryVisualState(stageData.m_sBoundaryEntityName, CRF_BoundaryStageState.ACTIVATED);
		
		// Play stage end sound if configured
		if (!m_sStageEndSound.IsEmpty())
		{
			m_sSoundToPlay = m_sStageEndSound;
			Replication.BumpMe();
		}
		
		// Show completion popup
		string completionMessage;
		if (!stageData.m_sStageCompletionMainMessage.IsEmpty())
		{
			string subMessage;
			if (stageData.m_sStageCompletionSubMessage.IsEmpty())
				subMessage = string.Format("Play area %1", stageTypeString);
			else
				subMessage = stageData.m_sStageCompletionSubMessage;
				
			completionMessage = string.Format("%1|%2|%3", 
				stageData.m_sStageCompletionMainMessage, 
				stageData.m_iStageCompletionDuration, 
				subMessage);
		}
		else
		{
			string defaultMessage;
			if (isChainedExecution)
				defaultMessage = "Stage %1 Complete";
			else
				defaultMessage = "Stage %1 Executed";
			int messageDuration;
			if (isChainedExecution)
				messageDuration = 10;
			else
				messageDuration = 8;
			completionMessage = string.Format("%1|%2|Play area %3", 
				string.Format(defaultMessage, stageIndex + 1), 
				messageDuration, 
				stageTypeString);
		}
		
		m_sMessageContent = completionMessage;
		Replication.BumpMe();
		ShowMessage();
		
		return stageTypeString;
	}

	//-------------------------------------------------------------------------------------------------
	// EXTERNAL SCRIPT ACCESS METHODS
	// These methods allow other scripts to manually control the staging system
	//-------------------------------------------------------------------------------------------------
	
	/**
	 * Trigger a specific stage by index
	 * 
	 * Starts the timer for the specified stage (if automatic timers enabled), 
	 * then executes the boundary action when timer completes.
	 * 
	 * @param stageIndex - Index of the stage to trigger (0-based)
	 * @return bool - true if stage was triggered successfully
	 * 
	 * USAGE EXAMPLES:
	 * 
	 * // Trigger stage 1 (index 0) - starts timer then executes
		CRF_MapStagingComponent staging = CRF_MapStagingComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_MapStagingComponent));
		staging.TriggerStage(0);
	 *
	 * // Perfect for objective-based triggers, entity destructors, or event-driven staging
	 * // If automatic timers disabled: only executes via this manual trigger
	 */
	bool TriggerStage(int stageIndex)
	{
		// Server-only operation for multiplayer safety
		if (RplSession.Mode() == RplMode.Client)
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] ManuallyTriggerStage called on client - ignoring");
			return false;
		}
		
		if (!m_aBoundaryStages || stageIndex < 0 || stageIndex >= m_aBoundaryStages.Count())
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] ManuallyTriggerStage failed - invalid stage index %1", stageIndex));
			return false;
		}
		
		CRF_BoundaryStageData stageData = m_aBoundaryStages[stageIndex];
		if (!stageData)
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] ManuallyTriggerStage failed - stage data is null at index %1", stageIndex));
			return false;
		}
		
		// Set current stage and activate staging
		m_iCurrentStage = stageIndex;
		m_bStagingActive = true;
		Replication.BumpMe(); // Replicate manual stage trigger
		
		// Play stage start sound if configured (for both timer and immediate execution)
		if (!m_sStageStartSound.IsEmpty())
		{
			m_sSoundToPlay = m_sStageStartSound;
			Replication.BumpMe();
		}
		
		// Check if automatic timers are enabled
		if (m_bUseAutomaticTimers)
		{
			// Use timer - start countdown then execute
			m_iCurrentTimer = stageData.m_iStageDuration;
			countDownActive = true;
			m_sStageText = stageData.m_sStageDisplayText;
			
			// Update boundary visual state to ACTIVE (timer starting)
			UpdateBoundaryVisualState(stageData.m_sBoundaryEntityName, CRF_BoundaryStageState.ACTIVE);
			
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Manually starting stage %1: '%2' with %3s timer", 
				stageIndex + 1, m_sStageText, m_iCurrentTimer));
			
			Replication.BumpMe();
			GetGame().GetCallqueue().CallLater(UpdateStageTimer, 1000, true);
		}
		else
		{
			// No timer - execute immediately but with proper sound timing
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Manually executing stage %1 immediately (no timer)", stageIndex + 1));
			
			// Set stage text for display (similar to timer path)
			m_sStageText = stageData.m_sStageDisplayText;
			
			// Update boundary visual state to ACTIVE briefly, then execute
			UpdateBoundaryVisualState(stageData.m_sBoundaryEntityName, CRF_BoundaryStageState.ACTIVE);
			Replication.BumpMe();
			
			// Delay ExecuteStage by 1 second to allow start sound to play properly
			// This prevents replication race condition between start and end sounds
			GetGame().GetCallqueue().CallLater(ExecuteStage, 1000, false);
		}
		
		return true;
	}
	
	/**
	 * Begin the staging system manually
	 * 
	 * Bypasses safestart monitoring and immediately begins the first stage.
	 * Useful for custom game mode initialization or testing.
	 * 
	 * USAGE EXAMPLE:
	 * 
		CRF_MapStagingComponent staging = CRF_MapStagingComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_MapStagingComponent));
		staging.BeginStaging();
	 */
	void BeginStaging()
	{
		// Server-only operation for multiplayer safety
		if (RplSession.Mode() == RplMode.Client)
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] BeginStaging called on client - ignoring");
			return;
		}
		
		if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] Manually starting staging system");
		m_bStagingActive = true;
		m_iCurrentStage = 0;
		Replication.BumpMe(); // Replicate manual staging start
		StartStaging();
	}
	
	/**
	 * Stop the staging system and clear all timers
	 * 
	 * Emergency stop function that halts all staging activity,
	 * clears active timers, and resets the system state.
	 * 
	 * USAGE EXAMPLE:
	 * 
		CRF_MapStagingComponent staging = CRF_MapStagingComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_MapStagingComponent));
		staging.StopStaging();
	 */
	void StopStaging()
	{
		// Server-only operation for multiplayer safety
		if (RplSession.Mode() == RplMode.Client)
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] StopStaging called on client - ignoring");
			return;
		}
		
		if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] Manually stopping staging system");
		m_bStagingActive = false;
		countDownActive = false;
		m_sStageText = "";
		m_sTimerText = ""; // Clear timer display
		Replication.BumpMe(); // Replicate staging stop
		GetGame().GetCallqueue().Remove(UpdateStageTimer);
		GetGame().GetCallqueue().Remove(MonitorSafestart);
		GetGame().GetCallqueue().Remove(StartStaging); // Prevent pending StartStaging calls
		GetGame().GetCallqueue().Remove(InitializeBoundaries); // Prevent pending initialization
	}
	
	/**
	 * Trigger a single stage immediately without chaining to next stages
	 * 
	 * Executes only the specified stage without triggering subsequent stages.
	 * Perfect for non-linear staging, objective-based triggers, or entity destructors.
	 * 
	 * @param stageIndex - Index of the stage to trigger (0-based)
	 * @return bool - true if stage was triggered successfully
	 * 
	 * USAGE EXAMPLES:
	 * 
	 * // Trigger only stage 2 without chaining to other stages
		CRF_MapStagingComponent staging = CRF_MapStagingComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_MapStagingComponent));
		staging.TriggerStageChainless(1);
	 *
	 * // Common use cases:
	 * // - Capture point A unlocks specific area (trigger stage 3)
	 * // - Destroy objective triggers boundary change
	 * // - Non-sequential stage progression based on player actions
	 */
	bool TriggerStageChainless(int stageIndex)
	{
		// Server-only operation for multiplayer safety
		if (RplSession.Mode() == RplMode.Client)
		{
			if (m_bDebugEnabled) Print("[CRF_MapStagingComponent] TriggerStageChainless called on client - ignoring");
			return false;
		}
		
		if (!m_aBoundaryStages || stageIndex < 0 || stageIndex >= m_aBoundaryStages.Count())
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] TriggerStageChainless failed - invalid stage index %1", stageIndex));
			return false;
		}
		
		CRF_BoundaryStageData stageData = m_aBoundaryStages[stageIndex];
		if (!stageData)
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] TriggerStageChainless failed - stage data is null at index %1", stageIndex));
			return false;
		}
		
		IEntity boundaryEntity = GetCachedBoundaryEntity(stageData.m_sBoundaryEntityName);
		if (!boundaryEntity)
		{
			if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] TriggerStageChainless failed - boundary entity '%1' not found", stageData.m_sBoundaryEntityName));
			return false;
		}
		
		// Execute using helper method (false = single execution, not chained)
		ExecuteStageBoundaryAction(stageData, stageIndex, false);
		
		if (m_bDebugEnabled) Print(string.Format("[CRF_MapStagingComponent] Single stage %1 triggered successfully - no chaining", stageIndex + 1));
		return true;
	}
}
