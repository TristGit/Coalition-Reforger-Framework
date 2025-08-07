/****************************************************************************************
 * File Name:        CRF_MapStagingComponent_Display
 * Author:           Assistant
 * Date Created:     08/03/25
 * Description:      HUD display for map staging system
 ****************************************************************************************/

class CRF_MapStagingComponentDisplay : SCR_InfoDisplayExtended
{
	protected string m_sStoredString;
	protected TextWidget m_wTimer;
	protected TextWidget m_wStageText;
	protected ImageWidget m_wBackground;
	protected CRF_MapStagingComponent m_StagingComponent;
	
	// Store original sizes and positions for scaling
	protected float m_fOriginalTimerWidth = -1;
	protected float m_fOriginalTimerHeight = -1;
	protected float m_fOriginalTimerX = 0;
	protected float m_fOriginalTimerY = 0;
	protected float m_fOriginalStageX = 0;
	protected float m_fOriginalStageY = 0;
	protected float m_fOriginalBgWidth = -1;
	protected float m_fOriginalBgHeight = -1;
	protected float m_fOriginalBgX = 0;
	protected float m_fOriginalBgY = 0;
	
	// Animation variables
	protected float m_fTargetScale = 1.0;
	protected float m_fPulseIntensity = 0;
	
	// Optimization: track last timer value to avoid redundant calculations
	protected int m_iLastTimeLeft = -1;
	
	// Smooth oscillation variables (visual effects only - no scaling)
	protected float m_fOscillationTime = 0;
	protected float m_fLastOscillationSpeed = 2.5; // Track last speed for smooth transitions
	
	// Sound alert tracking
	protected bool m_bTwentySecondSoundPlayed = false;
	protected bool m_bTenSecondSoundPlayed = false;
	
	//------------------------------------------------------------------------------------------------
	override protected void DisplayUpdate(IEntity owner, float timeSlice)
	{
		super.DisplayUpdate(owner, timeSlice);
		
		if (RplSession.Mode() == RplMode.Dedicated)
			return;
		
		// Get components if not initialized
		if (!m_StagingComponent || !m_wTimer || !m_wBackground || !m_wStageText) 
		{
			m_StagingComponent = CRF_MapStagingComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_MapStagingComponent));
			if (!m_StagingComponent) return;
			
			m_wTimer = TextWidget.Cast(m_wRoot.FindWidget("Timer"));
			m_wStageText = TextWidget.Cast(m_wRoot.FindWidget("StageText"));
			m_wBackground = ImageWidget.Cast(m_wRoot.FindWidget("Background"));
			
			if (!m_wTimer || !m_wStageText || !m_wBackground) return;
			
			// Store original sizes on first initialization
			if (m_fOriginalTimerWidth < 0)
			{
				// Use actual values from layout
				m_fOriginalTimerWidth = 96;
				m_fOriginalTimerHeight = 31;
				m_fOriginalBgWidth = 128;
				m_fOriginalBgHeight = 30;
				
				// Store original positions from layout file
				m_fOriginalTimerX = -48;
				m_fOriginalTimerY = 24;
				m_fOriginalStageX = -72;
				m_fOriginalStageY = 56.8;
				m_fOriginalBgX = -64;
				m_fOriginalBgY = 25;
			}
		}
		
		// Check HUD visibility
		if (!CRF_PlayerControllerManager.GetInstance().m_bHUDVisible)
		{
			HideDisplay();
			return;
		}
		
		// Check if countdown is active
		if (!m_StagingComponent.countDownActive)
		{
			HideDisplay();
			return;
		}
		
		// Show stage text
		UpdateStageText();
		
		// Update timer display
		UpdateTimer();
		
		// Update animations
		UpdateAnimations(timeSlice);
	}
	
	//------------------------------------------------------------------------------------------------
	void HideDisplay()
	{
		m_wTimer.SetOpacity(0);
		m_wStageText.SetOpacity(0);
		m_wBackground.SetOpacity(0);
		
		// Reset sound flags when display is hidden
		m_bTwentySecondSoundPlayed = false;
		m_bTenSecondSoundPlayed = false;
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateStageText()
	{
		if (!m_StagingComponent.m_sStageText.IsEmpty())
		{
			m_wStageText.SetText(m_StagingComponent.m_sStageText);
			m_wStageText.SetOpacity(1);
		}
		else
		{
			m_wStageText.SetOpacity(0);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateTimer()
	{
		// Display timer text from dedicated m_sTimerText property 
		if (!m_StagingComponent.m_sTimerText.IsEmpty())
		{
			if (m_sStoredString != m_StagingComponent.m_sTimerText) 
			{
				m_wTimer.SetText(m_StagingComponent.m_sTimerText);
				m_sStoredString = m_StagingComponent.m_sTimerText;
			}
			
			// Determine animation intensity based on timer value
			float newTargetScale = 1.0;
			float newPulseIntensity = 0;
			
			// Access the current timer value
			int timeLeft = m_StagingComponent.m_iCurrentTimer;
			
			// Only recalculate animation parameters if timer value changed
			if (timeLeft != m_iLastTimeLeft)
			{
				m_iLastTimeLeft = timeLeft;
				
				// Sound alerts
				if (timeLeft == 20 && !m_bTwentySecondSoundPlayed)
				{
					AudioSystem.PlaySound("{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav");
					m_bTwentySecondSoundPlayed = true;
				}
				else if (timeLeft == 10 && !m_bTenSecondSoundPlayed)
				{
					AudioSystem.PlaySound("{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav");
					m_bTenSecondSoundPlayed = true;
				}
				
				if (timeLeft <= 20 && timeLeft > 0)
				{
					// Reduced scale increase from 1.0 to 1.25 as we approach 0
					float progress = Math.InverseLerp(20, 1, timeLeft); // 0 at 20s, 1 at 1s
					newTargetScale = Math.Lerp(1.05, 1.25, progress);
					
					// Add pulse intensity that increases as time runs out (start at 20s)
					float pulseProgress = Math.InverseLerp(20, 1, timeLeft); // 0 at 20s, 1 at 1s
					newPulseIntensity = Math.Lerp(0.1, 0.8, pulseProgress); // Pulse opacity variation
				}
				
				m_fTargetScale = newTargetScale;
				m_fPulseIntensity = newPulseIntensity;
			}
			
			m_wTimer.SetOpacity(1);
			m_wBackground.SetOpacity(1);
		} 
		else 
		{
			m_wTimer.SetOpacity(0);
			m_wBackground.SetOpacity(0);
			
			// Reset animation state when timer is hidden
			m_fTargetScale = 1.0;
			m_fPulseIntensity = 0;
			m_fOscillationTime = 0;
			m_iLastTimeLeft = -1;
			
			// Reset sound flags when timer is not active
			m_bTwentySecondSoundPlayed = false;
			m_bTenSecondSoundPlayed = false;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateAnimations(float timeSlice)
	{
		// Update oscillation time at full framerate for smooth pulsing
		m_fOscillationTime += timeSlice;
		
		// Apply smooth pulsing effect if active
		if (m_fPulseIntensity > 0)
		{
			// Calculate oscillation speed that increases as timer approaches 0
			// At 20s: slow (2.5s cycle), at 1s: fast (0.5s cycle)
			float speedProgress = Math.InverseLerp(0.1, 0.8, m_fPulseIntensity); // 0 at low intensity, 1 at high intensity
			float oscillationSpeed = Math.Lerp(2.5, 0.5, speedProgress); // 2.5s cycle → 0.5s cycle
			
			// Smooth speed transition: adjust oscillation time to maintain phase continuity
			if (Math.AbsFloat(oscillationSpeed - m_fLastOscillationSpeed) > 0.01)
			{
				// Calculate current phase (0 to 2*PI within current cycle)
				float currentPhase = Math.Mod(m_fOscillationTime * Math.PI2 / m_fLastOscillationSpeed, Math.PI2);
				// Adjust oscillation time to maintain same phase with new speed
				m_fOscillationTime = (currentPhase * oscillationSpeed) / Math.PI2;
				m_fLastOscillationSpeed = oscillationSpeed;
			}
			
			// Calculate sine wave for smooth oscillation
			float sineWave = Math.Sin(m_fOscillationTime * Math.PI2 / oscillationSpeed);
			float normalizedWave = sineWave * 0.5 + 0.5; // Convert from [-1,1] to [0,1]
			
			// Calculate oscillating opacity and glow based on pulse intensity
			float currentOpacity = Math.Lerp(0.7, 1.0, normalizedWave * m_fPulseIntensity);
			float glowIntensity = Math.Lerp(0.2, 1.0, normalizedWave * m_fPulseIntensity);
			
			// Apply colors and opacity with smooth oscillation
			m_wTimer.SetColor(Color(0.053 + glowIntensity * 0.5, 0.578 + glowIntensity * 0.422, 0.053 + glowIntensity * 0.5, 1));
			m_wBackground.SetColor(Color(0.0894 + glowIntensity * 0.3, 0.0878 + glowIntensity * 0.4, 0.0878 + glowIntensity * 0.3, 0.333 + glowIntensity * 0.2));
			m_wTimer.SetOpacity(currentOpacity);
			m_wBackground.SetOpacity(currentOpacity);
		}
		else
		{
			// Reset to original colors and opacity
			m_wTimer.SetColor(Color(0.053, 0.578, 0.053, 1));
			m_wBackground.SetColor(Color(0.0894, 0.0878, 0.0878, 0.333));
			m_wTimer.SetOpacity(1.0);
			m_wBackground.SetOpacity(1.0);
			
			// Reset oscillation when not pulsing
			m_fOscillationTime = 0;
			m_fLastOscillationSpeed = 2.5; // Reset to initial speed
		}
	}
}