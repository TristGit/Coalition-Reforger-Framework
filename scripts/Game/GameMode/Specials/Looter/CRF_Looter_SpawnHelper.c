// CRF_Looter_SpawnHelper.c
// Spawn Methods for Loot - To be expanded on later - Automatic polyzone spawning later?

class CRF_LooterSpawnHelper
{
	// -----------------------------------
	// Static helper method for Loot spawning, for now is on empty object origin + height offset + spread + "flatten" rotation
	static void SpawnWithGroundPlacement(ResourceName prefabPath, EntitySpawnParams spawnParams, bool shouldLayFlat, int count = 1, float spreadRadius = 0.0)
	{
		for (int i = 0; i < count; i++)
		{
			// Calculate spawn position with optional spread paramters
			EntitySpawnParams finalParams = spawnParams;
			if (spreadRadius > 0.0)
			{
				vector offset = Vector(Math.RandomFloat(-spreadRadius, spreadRadius), 0, Math.RandomFloat(-spreadRadius, spreadRadius));
				finalParams.Transform[3] = spawnParams.Transform[3] + offset;
			}
			
			IEntity spawnedEntity = GetGame().SpawnEntityPrefab(Resource.Load(prefabPath), GetGame().GetWorld(), finalParams);
			
			// Apply height offset to prevent ground clipping
			if (spawnedEntity)
			{
				vector currentPos = spawnedEntity.GetOrigin();
				vector offsetPos = currentPos + "0 0.06 0"; // 5cm above surface
				spawnedEntity.SetOrigin(offsetPos);
			}
			
			// Apply flat rotation if loottype class specifies
			if (shouldLayFlat && spawnedEntity)
			{
				float randomYaw = Math.RandomInt(0, 360);
				vector rotation = Vector(randomYaw, 0, 90);
				spawnedEntity.SetYawPitchRoll(rotation);
			}
		}
	}
	
	// -----------------------------------
	// Convenience method for spawning multiple items with spread (like magazines)
	static void SpawnMultipleWithSpread(ResourceName prefabPath, EntitySpawnParams baseParams, int count, bool shouldLayFlat = true, float spreadRadius = 0.3)
	{
		SpawnWithGroundPlacement(prefabPath, baseParams, shouldLayFlat, count, spreadRadius);
	}
}
