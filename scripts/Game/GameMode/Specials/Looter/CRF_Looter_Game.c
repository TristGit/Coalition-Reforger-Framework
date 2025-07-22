// CRF_Looter_Game.c
// Meant to be for easy recreation of Loot in spcls or whatever, loot tables, and spawn logic
// Feel free to expand on this script or even rework it, meant to just be in a replicable gamecomponent
// can maybe at some point be remade into full gamemode with goal and win conditions
//=============================================================================
// LOOT ENTRY CLASSES - Define different types of loot with spawn behaviors
//=============================================================================

// -------------------------
// Base Class: CRF_BaseLootEntry
// -------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sPrimaryPrefab"}, "%1")]
class CRF_BaseLootEntry
{
	[Attribute(defvalue: "0.5", desc: "Drop weight (0.0–1.0 scale). Higher = more common.", params: "0 1 0.01")]
	float m_fWeight;

	[Attribute(defvalue: "", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", desc: "Primary item prefab")]
	ResourceName m_sPrimaryPrefab;
	
	// Internal flag - not visible in editor UI
	bool m_bIsFlat = false;
	
	// Virtual method to spawn items (overridden by derived classes)
	void SpawnLoot(EntitySpawnParams spawnParams)
	{
		// Always spawn primary item with ground placement and flat logic
		if (m_sPrimaryPrefab != "")
		{
			CRF_LooterSpawnHelper.SpawnWithGroundPlacement(m_sPrimaryPrefab, spawnParams, m_bIsFlat);
		}
	}
	
	string GetTypeName()
	{
		return this.Type().ToString();
	}
}

// -------------------------
// Class: CRF_WeaponLootEntry
// -------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_iMagazineCount", "m_sPrimaryPrefab"}, "%1 | %2")]
class CRF_WeaponLootEntry : CRF_BaseLootEntry
{
	[Attribute(defvalue: "{FB5EB0F6D447E858}Prefabs/Weapons/Magazines/Magazine_556x45_STANAG_30rnd_M193_Ball.et", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", desc: "Magazine prefab")]
	ResourceName m_sMagazinePrefab;

	[Attribute(defvalue: "0", desc: "Number of magazines to spawn")]
	int m_iMagazineCount;
	
	void CRF_WeaponLootEntry()
	{
		m_bIsFlat = true; // Weapons should lay flat
	}
	
	override void SpawnLoot(EntitySpawnParams spawnParams)
	{
		// Spawn weapon with ground placement
		if (m_sPrimaryPrefab != "")
		{
			CRF_LooterSpawnHelper.SpawnWithGroundPlacement(m_sPrimaryPrefab, spawnParams, m_bIsFlat);
		}
		
		// Spawn magazines with spread using unified helper method
		if (m_sMagazinePrefab != "" && m_sMagazinePrefab != "NONE" && m_iMagazineCount > 0)
		{
			CRF_LooterSpawnHelper.SpawnMultipleWithSpread(m_sMagazinePrefab, spawnParams, m_iMagazineCount);
		}
	}
}

// -------------------------
// Simple Loot Types: Gear/Bags
// ------------------------- 
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sPrimaryPrefab"}, "%1")]
class CRF_GearLootEntry : CRF_BaseLootEntry
{
	void CRF_GearLootEntry()
	{
		m_bIsFlat = false;
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sPrimaryPrefab"}, "%1")]
class CRF_BagLootEntry : CRF_BaseLootEntry
{
	void CRF_BagLootEntry()
	{
		m_bIsFlat = false;
	}
}

// -------------------------
// Multi-Item Loot Types: Kits and Misc Items
// -------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sPrefab", "m_iCount"}, "%1 %2")]
class CRF_KitItem
{
    [Attribute(defvalue: "", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", desc: "Item prefab")]
    ResourceName m_sPrefab;

    [Attribute(defvalue: "1", desc: "Number of this item to spawn")] 
    int m_iCount;
}
 
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sKitName"}, "%1")]
class CRF_KitLootEntry : CRF_BaseLootEntry
{
	[Attribute(defvalue: "Kit", desc: "Custom name for this kit (displayed in editor and debug)")]
	string m_sKitName;

	[Attribute(desc: "Kit Items")]
	ref array<ref CRF_KitItem> m_aKitItems;
	
	void CRF_KitLootEntry()
	{
		m_bIsFlat = false;
	}
	
	override void SpawnLoot(EntitySpawnParams spawnParams)
	{
		super.SpawnLoot(spawnParams);
		
		// Spawn kit items with individual counts and spread
		if (m_aKitItems)
		{
			foreach (CRF_KitItem kitItem : m_aKitItems)
			{
				if (kitItem && kitItem.m_sPrefab != "")
				{
					CRF_LooterSpawnHelper.SpawnMultipleWithSpread(kitItem.m_sPrefab, spawnParams, kitItem.m_iCount, false);
				}
			}
		}
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sPrefab", "m_iCount"}, "%1 %2")]
class CRF_MiscItem
{
    [Attribute(defvalue: "{C3F1FA1E2EC2B345}Prefabs/Items/Medicine/FieldDressing_01/FieldDressing_USSR_01.et", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", desc: "Item prefab")]
    ResourceName m_sPrefab;

    [Attribute(defvalue: "1", desc: "Number of this item to spawn")]
    int m_iCount;
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sPrimaryPrefab", "m_iPrimaryCount"}, "%1 %2")]
class CRF_MiscLootEntry : CRF_BaseLootEntry
{
    [Attribute(defvalue: "1", desc: "Number of primary items to spawn")]
    int m_iPrimaryCount;

    [Attribute(desc: "Extra Items")]
    ref array<ref CRF_MiscItem> m_aExtraItems;
    
    void CRF_MiscLootEntry()
    {
        m_bIsFlat = true; // Misc items like medical supplies often lay flat
    }
    
    override void SpawnLoot(EntitySpawnParams spawnParams)
    {
        // Spawn primary item(s) with count
        if (m_sPrimaryPrefab != "")
        {
            CRF_LooterSpawnHelper.SpawnWithGroundPlacement(m_sPrimaryPrefab, spawnParams, m_bIsFlat, m_iPrimaryCount);
        }
        
        // Spawn misc items with individual counts
        if (m_aExtraItems)
        {
            foreach (CRF_MiscItem miscItem : m_aExtraItems)
            {
                if (miscItem && miscItem.m_sPrefab != "")
                {
                    CRF_LooterSpawnHelper.SpawnWithGroundPlacement(miscItem.m_sPrefab, spawnParams, true, miscItem.m_iCount);
                }
            }
        }
    }
}

//=============================================================================
// MAIN GAMEMODE COMPONENT - Handles spawn logic, weighting, and distribution
//=============================================================================

[ComponentEditorProps(category: "Game Mode Component", description: "Spawns loot at named positions based on structured loot entries.")]
class CRF_LooterGamemodeComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_LooterGamemodeComponent : SCR_BaseGameModeComponent
{
	[Attribute("0.69", desc: "Chance (0.0–1.0) that a spawn point will spawn loot", params: "0 1 0.01")]
	protected float m_fGlobalSpawnChance;

	[Attribute("375", desc: "How many spawn points exist (named using the spawn point prefix + consecutive numbers)")]
	protected int m_iTotalSpawnPoints;

	[Attribute("lootSpawn", desc: "Prefix name for spawn points (ex: 'lootSpawn' then create empty game objects lootSpawn1, lootSpawn2, etc.)")]
	protected string m_sSpawnPointPrefix;

	[Attribute(defvalue: "false", desc: "Print individual item drop chances to debug log")]
	protected bool m_bDebugCalcItemChances;

	[Attribute(desc: "Weapon loot entries (weapons with magazines)")]
	ref array<ref CRF_WeaponLootEntry> m_aWeaponEntries;

	[Attribute(desc: "Gear loot entries (helmets, vests, etc.)")]
	ref array<ref CRF_GearLootEntry> m_aGearEntries;

	[Attribute(desc: "Bag loot entries (backpacks, pouches, etc.)")]
	ref array<ref CRF_BagLootEntry> m_aBagEntries;

	[Attribute(desc: "Kit loot entries (multi-item kits)")]
	ref array<ref CRF_KitLootEntry> m_aKitEntries;

	[Attribute(desc: "Misc loot entries (multiple random items)")]
	ref array<ref CRF_MiscLootEntry> m_aMiscEntries;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		if (!GetGame().InPlayMode())
			return;

		GetGame().GetCallqueue().CallLater(SpawnLoot, 3000, false);
		Print("CRF_LooterGamemodeComponent: Spawning loot...");
	}

	// Main spawning function - builds combined loot pool and spawns at random points
	protected void SpawnLoot()
	{
		// Build combined loot pool from all categories
		ref array<ref CRF_BaseLootEntry> allLootEntries = {};
		
		// Add all entries to combined pool (manual insertion for type safety)
		if (m_aWeaponEntries)  for (int i = 0; i < m_aWeaponEntries.Count(); i++)  allLootEntries.Insert(m_aWeaponEntries[i]);
		if (m_aGearEntries)    for (int i = 0; i < m_aGearEntries.Count(); i++)    allLootEntries.Insert(m_aGearEntries[i]);
		if (m_aBagEntries)     for (int i = 0; i < m_aBagEntries.Count(); i++)     allLootEntries.Insert(m_aBagEntries[i]);
		if (m_aKitEntries)     for (int i = 0; i < m_aKitEntries.Count(); i++)     allLootEntries.Insert(m_aKitEntries[i]);
		if (m_aMiscEntries)    for (int i = 0; i < m_aMiscEntries.Count(); i++)    allLootEntries.Insert(m_aMiscEntries[i]);
		
		// Skip spawning if no loot entries are configured (safety check)
		if (allLootEntries.Count() == 0)
		{
			Print("CRF_LooterGamemodeComponent: No loot entries configured, skipping spawn.");
			return;
		}

		// Calculate and optionally print drop chances
		if (m_bDebugCalcItemChances)
			PrintDropChances(allLootEntries);

		// Prepare spawn parameters once
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;

		// Build weight array and calculate total weight in one pass
		ref array<float> weights = {};
		float totalWeight = 0;
		foreach (CRF_BaseLootEntry entry : allLootEntries)
		{
			weights.Insert(entry.m_fWeight);
			totalWeight += entry.m_fWeight;
		}

		// Iterate through all spawn points
		for (int i = 1; i <= m_iTotalSpawnPoints; i++)
		{
			string spawnPointName = m_sSpawnPointPrefix + i.ToString();
			IEntity spawnPoint = GetGame().GetWorld().FindEntityByName(spawnPointName);
			if (!spawnPoint || Math.RandomFloat01() > m_fGlobalSpawnChance)
				continue;

			// Choose weighted random loot entry
			int chosenIndex = WeightedRandomIndex(weights, totalWeight);
			if (chosenIndex >= 0 && chosenIndex < allLootEntries.Count())
			{
				spawnParams.Transform[3] = spawnPoint.GetOrigin();
				allLootEntries[chosenIndex].SpawnLoot(spawnParams);
			}
		}
	}

	// Weighted random selection helper
	protected int WeightedRandomIndex(array<float> weights, float totalWeight)
	{
		float roll = Math.RandomFloat01() * totalWeight;
		float cumulative = 0.0;
		for (int i = 0; i < weights.Count(); i++)
		{
			cumulative += weights[i];
			if (roll <= cumulative)
				return i;
		}
		return -1;
	}

	// DEBUG OUTPUT - Print drop chances for all loot entries
	protected void PrintDropChances(array<ref CRF_BaseLootEntry> allLootEntries)
	{
		float totalWeight = 0;
		foreach (CRF_BaseLootEntry entry : allLootEntries)
		{
			totalWeight += entry.m_fWeight;
		}

		Print("=== CRF LOOT DROP CHANCES ===");
		Print(string.Format("Total configured items: %1", allLootEntries.Count()));
		Print(string.Format("Total weight: %1", totalWeight));
		
		foreach (CRF_BaseLootEntry entry : allLootEntries)
		{
			float percentage = (entry.m_fWeight / totalWeight) * 100;
			
			// Extract prefab name from path or use kit name for kits
			string displayName = "";
			CRF_KitLootEntry kitEntry = CRF_KitLootEntry.Cast(entry);
			if (kitEntry)
			{
				// Use custom kit name for kit entries
				displayName = kitEntry.m_sKitName;
			}
			else
			{
				// Extract prefab name from path for other entries
				string prefabName = entry.m_sPrimaryPrefab;
				if (prefabName != "")
				{
					int lastSlash = prefabName.LastIndexOf("/");
					if (lastSlash >= 0)
						displayName = prefabName.Substring(lastSlash + 1, prefabName.Length() - lastSlash - 1);
					else
						displayName = prefabName;
				}
				else
				{
					displayName = "Empty";
				}
			}
			
			// Use class name for type (removes CRF_ prefix)
			string typeName = entry.GetTypeName();
			if (typeName.IndexOf("CRF_") == 0)
				typeName = typeName.Substring(4, typeName.Length() - 4);
			
			Print(string.Format("CRF LOOT [%1] %2: %3%% chance (weight: %4)", typeName, displayName, percentage, entry.m_fWeight));
		}
		
		Print("=== END LOOT DROP CHANCES ===");
	}
}