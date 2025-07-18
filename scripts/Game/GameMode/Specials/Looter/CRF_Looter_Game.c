// CRF_Looter_Game.c
// Loot spawning gamemode component. Heavily translated via copilot into workable component.
// Feel free to modify and improve this, may be too overly engineered

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
	
	// Virtual method to spawn items (overridden by derived classes)
	void SpawnLoot(EntitySpawnParams spawnParams)
	{
		// Always spawn primary item
		if (m_sPrimaryPrefab != "")
		{
			GetGame().SpawnEntityPrefab(Resource.Load(m_sPrimaryPrefab), GetGame().GetWorld(), spawnParams);
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
	
	override void SpawnLoot(EntitySpawnParams spawnParams)
	{
		super.SpawnLoot(spawnParams);
		
		// Spawn magazines for weapons
		if (m_sMagazinePrefab != "" && m_sMagazinePrefab != "NONE")
		{
			for (int i = 0; i < m_iMagazineCount; i++)
			{
				GetGame().SpawnEntityPrefab(Resource.Load(m_sMagazinePrefab), GetGame().GetWorld(), spawnParams);
			}
		}
	}
}

// -------------------------
// Class: CRF_GearLootEntry
// -------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sPrimaryPrefab"}, "%1")]
class CRF_GearLootEntry : CRF_BaseLootEntry
{
	// This is just for the seperate gear class, uses inherited class
}

// -------------------------
// Helper Class: CRF_KitItem
// -------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sPrefab", "m_iCount"}, "%1 (%2)")]
class CRF_KitItem
{
    [Attribute(defvalue: "", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", desc: "Item prefab")]
    ResourceName m_sPrefab;

    [Attribute(defvalue: "1", desc: "Number of this item to spawn")]
    int m_iCount;
}

// -------------------------
// Class: CRF_KitLootEntry
// -------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sKitName"}, "%1")]
class CRF_KitLootEntry : CRF_BaseLootEntry
{
	[Attribute(defvalue: "Kit", desc: "Custom name for this kit (displayed in editor and debug)")]
	string m_sKitName;

	[Attribute(desc: "Kit Items")]
	ref array<ref CRF_KitItem> m_aKitItems;
	
	override void SpawnLoot(EntitySpawnParams spawnParams)
	{
		super.SpawnLoot(spawnParams);
		
		// Spawn kit items with individual counts
		if (m_aKitItems)
		{
			foreach (CRF_KitItem kitItem : m_aKitItems)
			{
				if (kitItem && kitItem.m_sPrefab != "")
				{
					for (int i = 0; i < kitItem.m_iCount; i++)
					{
						GetGame().SpawnEntityPrefab(Resource.Load(kitItem.m_sPrefab), GetGame().GetWorld(), spawnParams);
					}
				}
			}
		}
	}
}

// -------------------------
// Helper Class: CRF_MiscItem
// -------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sPrefab", "m_iCount"}, "%1 (%2)")]
class CRF_MiscItem
{
    [Attribute(defvalue: "{C3F1FA1E2EC2B345}Prefabs/Items/Medicine/FieldDressing_01/FieldDressing_USSR_01.et", uiwidget: UIWidgets.ResourcePickerThumbnail, params: "et", desc: "Item prefab")]
    ResourceName m_sPrefab;

    [Attribute(defvalue: "1", desc: "Number of this item to spawn")]
    int m_iCount;
}

// -------------------------
// Class: CRF_MiscLootEntry
// -------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleFields({"m_sPrimaryPrefab", "m_iPrimaryCount"}, "%1 (%2)")]
class CRF_MiscLootEntry : CRF_BaseLootEntry
{
    [Attribute(defvalue: "1", desc: "Number of primary items to spawn")]
    int m_iPrimaryCount;

    [Attribute(desc: "Extra Items")]
    ref array<ref CRF_MiscItem> m_aExtraItems;
    
    override void SpawnLoot(EntitySpawnParams spawnParams)
    {
        // Spawn primary item(s) with count
        if (m_sPrimaryPrefab != "")
        {
            for (int i = 0; i < m_iPrimaryCount; i++)
            {
                GetGame().SpawnEntityPrefab(Resource.Load(m_sPrimaryPrefab), GetGame().GetWorld(), spawnParams);
            }
        }
        
        // Spawn misc items with individual counts
        if (m_aExtraItems)
        {
            foreach (CRF_MiscItem miscItem : m_aExtraItems)
            {
                if (miscItem && miscItem.m_sPrefab != "")
                {
                    for (int j = 0; j < miscItem.m_iCount; j++)
                    {
                        GetGame().SpawnEntityPrefab(Resource.Load(miscItem.m_sPrefab), GetGame().GetWorld(), spawnParams);
                    }
                }
            }
        }
    }
}

// -------------------------
// Class: CRF_LooterGamemodeComponent
// -------------------------
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

	[Attribute(desc: "Kit loot entries (multi-item kits)")]
	ref array<ref CRF_KitLootEntry> m_aKitEntries;

	[Attribute(desc: "Misc loot entries (multiple random items)")]
	ref array<ref CRF_MiscLootEntry> m_aMiscEntries;

	// ---------------- executing on postinit, hope formatted correctly
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		if (!GetGame().InPlayMode())
			return;

		GetGame().GetCallqueue().CallLater(SpawnLoot, 3000, false);
		Print("CRF_LooterGamemodeComponent: Spawning loot...");
	}

	// ----------------- Main spawning function
	protected void SpawnLoot()
	{
		// Build combined loot pool from all categories
		ref array<ref CRF_BaseLootEntry> allLootEntries = {};
		
		// Add all weapon entries
		if (m_aWeaponEntries)
		{
			foreach (CRF_WeaponLootEntry weaponEntry : m_aWeaponEntries)
			{
				allLootEntries.Insert(weaponEntry);
			}
		}
		
		// Add all gear entries
		if (m_aGearEntries)
		{
			foreach (CRF_GearLootEntry gearEntry : m_aGearEntries)
			{
				allLootEntries.Insert(gearEntry);
			}
		}
		
		// Add all kit entries
		if (m_aKitEntries)
		{
			foreach (CRF_KitLootEntry kitEntry : m_aKitEntries)
			{
				allLootEntries.Insert(kitEntry);
			}
		}
		
		// Add all misc entries
		if (m_aMiscEntries)
		{
			foreach (CRF_MiscLootEntry miscEntry : m_aMiscEntries)
			{
				allLootEntries.Insert(miscEntry);
			}
		}
		
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

	// -----------------------------------
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

	// ------------------ Debug / calculation logging related from here on
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