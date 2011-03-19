/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/demo.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>
#include <engine/shared/config.h>

#include <game/gamecore.h> // get_angle
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/render.h>

#include <game/client/components/flow.h>
#include <game/client/components/effects.h>

#include "items.h"

void CItems::RenderProjectile(const CNetObj_Projectile *pCurrent, int ItemID)
{

	// get positions
	float Curvature = 0;
	float Speed = 0;
	if(pCurrent->m_Type == WEAPON_GRENADE)
	{
		Curvature = m_pClient->m_Tuning.m_GrenadeCurvature;
		Speed = m_pClient->m_Tuning.m_GrenadeSpeed;
	}
	else if(pCurrent->m_Type == WEAPON_SHOTGUN)
	{
		Curvature = m_pClient->m_Tuning.m_ShotgunCurvature;
		Speed = m_pClient->m_Tuning.m_ShotgunSpeed;
	}
	else if(pCurrent->m_Type == WEAPON_GUN)
	{
		Curvature = m_pClient->m_Tuning.m_GunCurvature;
		Speed = m_pClient->m_Tuning.m_GunSpeed;
	}

	float Ct = (Client()->PrevGameTick()-pCurrent->m_StartTick)/(float)SERVER_TICK_SPEED + Client()->GameTickTime();
	if(Ct < 0)
		return; // projectile havn't been shot yet
		
	vec2 StartPos(pCurrent->m_X, pCurrent->m_Y);
	vec2 StartVel(pCurrent->m_VelX/100.0f, pCurrent->m_VelY/100.0f);
	vec2 Pos = CalcPos(StartPos, StartVel, Curvature, Speed, Ct);
	vec2 PrevPos = CalcPos(StartPos, StartVel, Curvature, Speed, Ct-0.001f);


	// Draw shadows of grenades
	bool LocalPlayerInGame = m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_Team != -1;
	int explode = 0; // explode detecting
	
	if(g_Config.m_AntiPing && g_Config.m_AntiPingGrenade && g_Config.m_AntiPingOnlyIfBigLatency && (g_Config.m_AntiPingLatency <= m_pClient->m_Snap.m_pLocalInfo->m_Latency) && pCurrent->m_Type == WEAPON_GRENADE && LocalPlayerInGame && !m_pClient->m_Snap.m_pGameobj->m_GameOver)
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		Graphics()->QuadsBegin();
		
		// Calculate average prediction offset, because client_predtick() gets varial values :(((
		// Must be there is a normal way to realize it, but I'm too lazy to find it. 
		if (m_pClient->m_Average_Prediction_Offset == -1)
		{
			int Offset = Client()->PredGameTick() - Client()->GameTick();
			m_pClient->m_Prediction_Offset_Summ += Offset;
			m_pClient->m_Prediction_Offset_Count++;
			
			if (m_pClient->m_Prediction_Offset_Count >= 100)
			{
				m_pClient->m_Average_Prediction_Offset = round((float)m_pClient->m_Prediction_Offset_Summ / m_pClient->m_Prediction_Offset_Count);
			}
		}
		
		// Draw shadow only if grenade directed to local player (optionaly)
		int LocalCid = m_pClient->m_Snap.m_LocalClientID;
		CNetObj_CharacterCore& CurChar = m_pClient->m_Snap.m_aCharacters[LocalCid].m_Cur;
		CNetObj_CharacterCore& PrevChar = m_pClient->m_Snap.m_aCharacters[LocalCid].m_Prev;
		vec2 ServerPos = mix(vec2(PrevChar.m_X, PrevChar.m_Y), vec2(CurChar.m_X, CurChar.m_Y), Client()->IntraGameTick());
		
		bool GrenadeIsDirectedToLocalPlayer = true; 
		if (g_Config.m_AntiPingGrenadeSide)
		{
			float d1 = fabs(distance(Pos, ServerPos));
			float d2 = fabs(distance(PrevPos, ServerPos));
			bool GrenadeIsDirectedToLocalPlayer = d1 < d2;
		}
		
		// Detect explode
		int PredictedTick = Client()->PrevGameTick() + m_pClient->m_Average_Prediction_Offset;
		float PredictedCt = (PredictedTick - pCurrent->m_StartTick)/(float)SERVER_TICK_SPEED + Client()->GameTickTime();
		vec2 PredictedPos;
		vec2 PrevPredictedPos;
		
		if (g_Config.m_AntiPingGrenadeExpl)
		{
			// grenade explode on collisions
			float eps=g_Config.m_AntiPingGrenadeEps*0.0001f;
			for (int i=0; (Ct+i*eps<PredictedCt)&&!explode; i++)
			{
				PredictedPos = CalcPos(StartPos, StartVel, Curvature, Speed, Ct+i*eps);
				PrevPredictedPos = CalcPos(StartPos, StartVel, Curvature, Speed,  Ct+(i-1)*eps);
				if (Collision()->IntersectLine(PrevPredictedPos, PredictedPos, &PredictedPos, 0))
					explode=1;
			}
			
			// grenade explode if TimeSpan>GrenadeLifetime
			if (((PredictedTick-pCurrent->m_StartTick)/(float)SERVER_TICK_SPEED)>m_pClient->m_Tuning.m_GrenadeLifetime)
				explode=2;
				
			// Render explode
			if (explode==1)
			{
				m_pClient->m_pEffects->FakeExplosion(PredictedPos);
			}
			else if (explode==2)
			{
				float ExplodeCt=m_pClient->m_Tuning.m_GrenadeLifetime;
				PredictedPos = CalcPos(StartPos, StartVel, Curvature, Speed, ExplodeCt);
				m_pClient->m_pEffects->FakeExplosion(PredictedPos);
			}
		}
		
		if (m_pClient->m_Average_Prediction_Offset != -1 && GrenadeIsDirectedToLocalPlayer && !explode && PredictedCt >= 0)
		{
			PredictedPos = CalcPos(StartPos, StartVel, Curvature, Speed, PredictedCt);
			PrevPredictedPos = CalcPos(StartPos, StartVel, Curvature, Speed, PredictedCt-0.001f);
			int shadow_type = pCurrent->m_Type; 
			Graphics()->SetColor(0, 1, 0, 0.75f); 
			RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[clamp(shadow_type, 0, NUM_WEAPONS-1)].m_pSpriteProj);
			IGraphics::CQuadItem QuadItem(PredictedPos.x, PredictedPos.y, 28, 28);
			Graphics()->QuadsDraw(&QuadItem, 1);
		}
		
		Graphics()->QuadsSetRotation(0);
		Graphics()->QuadsEnd();
	}
	
	if (!(g_Config.m_AntiPingShowGrenadeIfExplode || explode==0 || pCurrent->m_Type != WEAPON_GRENADE))
		return;
	
	// draw original particle
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();
	
	RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[clamp(pCurrent->m_Type, 0, NUM_WEAPONS-1)].m_pSpriteProj);
	vec2 Vel = Pos-PrevPos;
	//vec2 pos = mix(vec2(prev->x, prev->y), vec2(current->x, current->y), Client()->IntraGameTick());
	

	// add particle for this projectile
	if(pCurrent->m_Type == WEAPON_GRENADE)
	{
		m_pClient->m_pEffects->SmokeTrail(Pos, Vel*-1);
		m_pClient->m_pFlow->Add(Pos, Vel*1000*Client()->FrameTime(), 10.0f);
		
		if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
		{
			const IDemoPlayer::CInfo *pInfo = DemoPlayer()->BaseInfo();
			static float Time = 0;
			static float LastLocalTime = Client()->LocalTime();
		
			if(!pInfo->m_Paused)
				Time += (Client()->LocalTime()-LastLocalTime)*pInfo->m_Speed;
			
			Graphics()->QuadsSetRotation(Time*pi*2*2 + ItemID);
			
			LastLocalTime = Client()->LocalTime();
		}
		else
			Graphics()->QuadsSetRotation(Client()->LocalTime()*pi*2*2 + ItemID);
	}
	else
	{
		if(pCurrent->m_Type != WEAPON_SHOTGUN)
			m_pClient->m_pEffects->SgBulletTrail(Pos);
		else
			m_pClient->m_pEffects->BulletTrail(Pos);
		m_pClient->m_pFlow->Add(Pos, Vel*1000*Client()->FrameTime(), 10.0f);

		if(length(Vel) > 0.00001f)
			Graphics()->QuadsSetRotation(GetAngle(Vel));
		else
			Graphics()->QuadsSetRotation(0);

	}

	IGraphics::CQuadItem QuadItem(Pos.x, Pos.y, 32, 32);
	Graphics()->QuadsDraw(&QuadItem, 1);
	Graphics()->QuadsSetRotation(0);
	Graphics()->QuadsEnd();
}

void CItems::RenderPickup(const CNetObj_Pickup *pPrev, const CNetObj_Pickup *pCurrent)
{
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();
	vec2 Pos = mix(vec2(pPrev->m_X, pPrev->m_Y), vec2(pCurrent->m_X, pCurrent->m_Y), Client()->IntraGameTick());
	float Angle = 0.0f;
	float Size = 64.0f;
	if (pCurrent->m_Type == POWERUP_WEAPON)
	{
		Angle = 0; //-pi/6;//-0.25f * pi * 2.0f;
		RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[clamp(pCurrent->m_Subtype, 0, NUM_WEAPONS-1)].m_pSpriteBody);
		Size = g_pData->m_Weapons.m_aId[clamp(pCurrent->m_Subtype, 0, NUM_WEAPONS-1)].m_VisualSize;
		m_pClient->m_pEffects->WeaponShine(Pos, vec2(96,18));
	}
	else
	{
		const int c[] = {
			SPRITE_PICKUP_HEALTH,
			SPRITE_PICKUP_ARMOR,
			SPRITE_PICKUP_WEAPON,
			SPRITE_PICKUP_NINJA
			};
		RenderTools()->SelectSprite(c[pCurrent->m_Type]);

		if(c[pCurrent->m_Type] == SPRITE_PICKUP_NINJA)
		{
			m_pClient->m_pEffects->PowerupShine(Pos, vec2(96,18));
			Size *= 2.0f;
			Pos.x -= 10.0f;
		}
	}

	Graphics()->QuadsSetRotation(Angle);

	float Offset = Pos.y/32.0f + Pos.x/32.0f;
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		const IDemoPlayer::CInfo *pInfo = DemoPlayer()->BaseInfo();
		static float Time = 0;
		static float LastLocalTime = Client()->LocalTime();
		
		if(!pInfo->m_Paused)
			Time += (Client()->LocalTime()-LastLocalTime)*pInfo->m_Speed;
			
		Pos.x += cosf(Time*2.0f+Offset)*2.5f;
		Pos.y += sinf(Time*2.0f+Offset)*2.5f;
		
		LastLocalTime = Client()->LocalTime();
	}
	else
	{
		Pos.x += cosf(Client()->LocalTime()*2.0f+Offset)*2.5f;
		Pos.y += sinf(Client()->LocalTime()*2.0f+Offset)*2.5f;
	}
	RenderTools()->DrawSprite(Pos.x, Pos.y, Size);
	Graphics()->QuadsEnd();
}

void CItems::RenderFlag(const CNetObj_Flag *pPrev, const CNetObj_Flag *pCurrent)
{
	float Angle = 0.0f;
	float Size = 42.0f;

	Graphics()->BlendNormal();
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();

	Graphics()->QuadsSetRotation(Angle);

	vec2 Pos = mix(vec2(pPrev->m_X, pPrev->m_Y), vec2(pCurrent->m_X, pCurrent->m_Y), Client()->IntraGameTick());
	
	// make sure that the flag isn't interpolated between capture and return
	if(pPrev->m_CarriedBy != pCurrent->m_CarriedBy)
		Pos = vec2(pCurrent->m_X, pCurrent->m_Y);

	// make sure to use predicted position if we are the carrier
	if(m_pClient->m_Snap.m_pLocalInfo && pCurrent->m_CarriedBy == m_pClient->m_Snap.m_pLocalInfo->m_ClientID)
		Pos = m_pClient->m_LocalCharacterPos;
		
	if(pCurrent->m_Team == TEAM_RED)
	{
		RenderTools()->SelectSprite(SPRITE_FLAG_RED);
		m_pClient->m_pEffects->RedFlagShine(Pos, vec2(96,18));
	} else {
		RenderTools()->SelectSprite(SPRITE_FLAG_BLUE);
		m_pClient->m_pEffects->BlueFlagShine(Pos, vec2(96,18));
	}

	IGraphics::CQuadItem QuadItem(Pos.x, Pos.y-Size*0.75f, Size, Size*2);
	Graphics()->QuadsDraw(&QuadItem, 1);
	Graphics()->QuadsEnd();
}


void CItems::RenderLaser(const struct CNetObj_Laser *pCurrent)
{
	vec2 Pos = vec2(pCurrent->m_X, pCurrent->m_Y);
	vec2 From = vec2(pCurrent->m_FromX, pCurrent->m_FromY);
	vec2 Dir = normalize(Pos-From);

	float Ticks = Client()->GameTick() + Client()->IntraGameTick() - pCurrent->m_StartTick;
	float Ms = (Ticks/50.0f) * 1000.0f;
	float a =  Ms / m_pClient->m_Tuning.m_LaserBounceDelay;
	a = clamp(a, 0.0f, 1.0f);
	float Ia = 1-a;
	
	vec2 Out, Border;
	
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	
	//vec4 inner_color(0.15f,0.35f,0.75f,1.0f);
	//vec4 outer_color(0.65f,0.85f,1.0f,1.0f);

	// do outline
	vec4 OuterColor(0.075f, 0.075f, 0.25f, 1.0f);
	Graphics()->SetColor(OuterColor.r, OuterColor.g, OuterColor.b, 1.0f);
	Out = vec2(Dir.y, -Dir.x) * (7.0f*Ia);

	IGraphics::CFreeformItem Freeform(
			From.x-Out.x, From.y-Out.y,
			From.x+Out.x, From.y+Out.y,
			Pos.x-Out.x, Pos.y-Out.y,
			Pos.x+Out.x, Pos.y+Out.y);
	Graphics()->QuadsDrawFreeform(&Freeform, 1);

	// do inner	
	vec4 InnerColor(0.5f, 0.5f, 1.0f, 1.0f);
	Out = vec2(Dir.y, -Dir.x) * (5.0f*Ia);
	Graphics()->SetColor(InnerColor.r, InnerColor.g, InnerColor.b, 1.0f); // center
	
	Freeform = IGraphics::CFreeformItem(
			From.x-Out.x, From.y-Out.y,
			From.x+Out.x, From.y+Out.y,
			Pos.x-Out.x, Pos.y-Out.y,
			Pos.x+Out.x, Pos.y+Out.y);
	Graphics()->QuadsDrawFreeform(&Freeform, 1);
		
	Graphics()->QuadsEnd();
	
	// render head
	{
		Graphics()->BlendNormal();
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_PARTICLES].m_Id);
		Graphics()->QuadsBegin();

		int Sprites[] = {SPRITE_PART_SPLAT01, SPRITE_PART_SPLAT02, SPRITE_PART_SPLAT03};
		RenderTools()->SelectSprite(Sprites[Client()->GameTick()%3]);
		Graphics()->QuadsSetRotation(Client()->GameTick());
		Graphics()->SetColor(OuterColor.r, OuterColor.g, OuterColor.b, 1.0f);
		IGraphics::CQuadItem QuadItem(Pos.x, Pos.y, 24, 24);
		Graphics()->QuadsDraw(&QuadItem, 1);
		Graphics()->SetColor(InnerColor.r, InnerColor.g, InnerColor.b, 1.0f);
		QuadItem = IGraphics::CQuadItem(Pos.x, Pos.y, 20, 20);
		Graphics()->QuadsDraw(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
	
	Graphics()->BlendNormal();	
}

void CItems::OnRender()
{
	int Num = Client()->SnapNumItems(IClient::SNAP_CURRENT);
	for(int i = 0; i < Num; i++)
	{
		IClient::CSnapItem Item;
		const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

		if(Item.m_Type == NETOBJTYPE_PROJECTILE)
		{
			RenderProjectile((const CNetObj_Projectile *)pData, Item.m_ID);
		}
		else if(Item.m_Type == NETOBJTYPE_PICKUP)
		{
			const void *pPrev = Client()->SnapFindItem(IClient::SNAP_PREV, Item.m_Type, Item.m_ID);
			if(pPrev)
				RenderPickup((const CNetObj_Pickup *)pPrev, (const CNetObj_Pickup *)pData);
		}
		else if(Item.m_Type == NETOBJTYPE_LASER)
		{
			RenderLaser((const CNetObj_Laser *)pData);
		}
	}

	// render flag
	for(int i = 0; i < Num; i++)
	{
		IClient::CSnapItem Item;
		const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

		if(Item.m_Type == NETOBJTYPE_FLAG)
		{
			const void *pPrev = Client()->SnapFindItem(IClient::SNAP_PREV, Item.m_Type, Item.m_ID);
			if (pPrev)
				RenderFlag((const CNetObj_Flag *)pPrev, (const CNetObj_Flag *)pData);
		}
	}

	// render extra projectiles
	/*
	for(int i = 0; i < extraproj_num; i++)
	{
		if(extraproj_projectiles[i].start_tick < Client()->GameTick())
		{
			extraproj_projectiles[i] = extraproj_projectiles[extraproj_num-1];
			extraproj_num--;
		}
		else
			render_projectile(&extraproj_projectiles[i], 0);
	}*/
}
