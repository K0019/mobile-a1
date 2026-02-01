function OnCollisionEnter(enteringEntity)
	Magic.AudioManager.PlaySound("mc hurt "..(Magic.Random.RangeInt(0,5)+1), false, Magic.AudioType.SFX)
end
