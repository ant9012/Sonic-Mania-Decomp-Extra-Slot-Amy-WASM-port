// ---------------------------------------------------------------------
// RSDK Project: Sonic Mania
// Object Description: Player Object
// Object Author: Christian Whitehead/Simon Thomley/Hunter Bridges
// Decompiled by: Rubberduckycooly & RMGRich
// ---------------------------------------------------------------------

#include "Game.h"

ObjectPlayer *Player;

Hitbox Player_FallbackHitbox = { -10, -20, 10, 20 };

void Player_Update(void)
{
    RSDK_THIS(Player);
    int32 medals = (globals->medalMods & MEDAL_AMYCDR);

#if MANIA_USE_PLUS
    // Cheat prevention, you can't play as mighty, ray, or amy if you don't have plus installed & active
    if (!API.CheckDLC(DLC_PLUS) && self->characterID > ID_KNUCKLES)
        Player_ChangeCharacter(self, ID_SONIC);

    StateMachine_Run(self->stateInputReplay);
#endif

    StateMachine_Run(self->stateInput);

    if (self->classID == Player->classID) {
        if (self->camera) {
            if (self->scrollDelay > 0) {
                self->scrollDelay--;
                if (!self->scrollDelay)
                    self->camera->state = Camera_State_FollowXY;
            }
            else if (self->state != Player_State_LookUp && self->state != Player_State_Crouch) {
                if (self->camera->lookPos.y > 0)
                    self->camera->lookPos.y -= 2;
                else if (self->camera->lookPos.y < 0)
                    self->camera->lookPos.y += 2;
            }
        }

        if (self->invincibleTimer) {
            if (self->invincibleTimer <= 0) {
                self->invincibleTimer++;
            }
            else {
                self->invincibleTimer--;
                if (!self->invincibleTimer) {
                    Player_ApplyShield(self);

#if MANIA_USE_PLUS
                    if (globals->gameMode != MODE_ENCORE || !self->sidekick) {
#else
                    if (!self->sidekick) {
#endif
                        bool32 stopPlaying = true;
                        foreach_active(Player, player)
                        {
                            if (player->invincibleTimer > 0)
                                stopPlaying = false;
                        }

                        if (stopPlaying)
                            Music_JingleFadeOut(TRACK_INVINCIBLE, true);
                    }
                }
            }
        }

        if (self->speedShoesTimer > 0) {
            self->speedShoesTimer--;
            if (!self->speedShoesTimer) {
                Player_UpdatePhysicsState(self);

                bool32 stopPlaying = true;
                foreach_active(Player, player)
                {
                    if (player->speedShoesTimer > 0)
                        stopPlaying = false;
                }

                if (stopPlaying)
                    Music_JingleFadeOut(TRACK_SNEAKERS, true);
            }
        }

        if (self->state != Player_State_Hurt) {
            if (self->blinkTimer > 0) {
                self->blinkTimer--;
                self->visible = !(self->blinkTimer & 4);
            }
        }

#if MANIA_USE_PLUS
        if (self->characterID == ID_RAY && self->state != Player_State_RayGlide && !self->isGhost) {
            Player->raySwoopTimer = 0;
            Player->rayDiveTimer  = 0;
        }

        if (self->uncurlTimer > 0)
            self->uncurlTimer--;
#endif

        Player_HandleSuperForm();
        if (self->characterID == ID_TAILS && self->state != Player_State_TailsFlight && self->abilitySpeed)
            self->abilitySpeed = 0;

        // Hurt Player if we're touching T/B or L/R sides at same time
        if (self->collisionFlagH == (1 | 2) || self->collisionFlagV == (1 | 2))
            self->deathType = PLAYER_DEATH_DIE_USESFX;

        self->collisionFlagH = 0;
        self->collisionFlagV = 0;
        if (self->collisionLayers & Zone->moveLayerMask) {
            TileLayer *move  = RSDK.GetTileLayer(Zone->moveLayer);
            move->position.x = -self->moveLayerPosition.x >> 16;
            move->position.y = -self->moveLayerPosition.y >> 16;
        }

        if (self->forceRespawn)
            self->state = Player_State_HoldRespawn;

#if GAME_VERSION != VER_100
        if (self->isTransforming)
            self->state = Player_State_Transform;
#endif

        StateMachine_Run(self->state);

        if (self->classID == Player->classID) {
            self->flailing      = false;
            self->invertGravity = false;

            if (self->outerbox) {
                self->groundedStore = self->onGround;
                RSDK.ProcessObjectMovement(self, self->outerbox, self->innerbox);
            }
            else {
                self->outerbox      = RSDK.GetHitbox(&self->animator, 0);
                self->innerbox      = RSDK.GetHitbox(&self->animator, 1);
                self->groundedStore = self->onGround;
                RSDK.ProcessObjectMovement(self, self->outerbox, self->innerbox);
                self->outerbox = NULL;
            }

            self->collisionLayers &= ~Zone->moveLayerMask;
            if (self->onGround && !self->collisionMode)
                self->collisionFlagV |= 1;
        }
    }
}

void Player_LateUpdate(void)
{
    RSDK_THIS(Player);

    if (self->superState == SUPERSTATE_FADEIN && self->state != Player_State_Transform)
        Player_TryTransform(self, 0x7F);

    if (self->state == Player_State_FlyCarried) {
        self->flyCarryLeaderPos.x = self->position.x & 0xFFFF0000;
        self->flyCarryLeaderPos.y = self->position.y & 0xFFFF0000;
    }

    if (self->deathType) {
        self->abilityValues[0] = 0;

#if MANIA_USE_PLUS
        if (!self->sidekick)
            RSDK.CopyEntity(Zone->entityStorage[1], self, false);
#endif

        bool32 stopInvincibility = self->sidekick || globals->gameMode == MODE_COMPETITION;
#if MANIA_USE_PLUS
        stopInvincibility |= globals->gameMode == MODE_ENCORE;
#endif

        if (stopInvincibility) {
            if (self->invincibleTimer > 1)
                self->invincibleTimer = 1;
            if (self->speedShoesTimer > 1)
                self->speedShoesTimer = 1;
        }
        else {
            self->invincibleTimer = 0;
            self->speedShoesTimer = 0;
        }

        if (self->gravityStrength <= 1)
            self->direction |= FLIP_Y;
        else
            self->direction &= ~FLIP_Y;

        self->visible         = true;
        self->onGround        = false;
        self->groundVel       = 0;
        self->velocity.x      = 0;
        self->nextGroundState = StateMachine_None;
        self->nextAirState    = StateMachine_None;
        self->interaction     = false;
        self->tileCollisions  = TILECOLLISION_NONE;

        if (globals->gameMode != MODE_COMPETITION)
            self->active = ACTIVE_ALWAYS;

        self->shield         = SHIELD_NONE;
        self->collisionFlagH = 0;
        self->collisionFlagV = 0;
        self->underwater     = false;
        Player_UpdatePhysicsState(self);

        destroyEntity(RSDK_GET_ENTITY(Player->playerCount + RSDK.GetEntitySlot(self), Shield));

        switch (self->deathType) {
            default: break;

            case PLAYER_DEATH_DIE_USESFX:
                RSDK.PlaySfx(Player->sfxHurt, false, 255);
                // [Fallthrough]
            case PLAYER_DEATH_DIE_NOSFX:
                self->deathType  = PLAYER_DEATH_NONE;
                self->velocity.y = -0x68000;
                self->state      = Player_State_Death;

                if (!(self->drawFX & FX_SCALE) || self->scale.x == 0x200)
                    self->drawGroup = Zone->playerDrawGroup[1];

                if (self->sidekick || globals->gameMode == MODE_COMPETITION) {
                    if (self->camera) {
                        self->scrollDelay   = 2;
                        self->camera->state = StateMachine_None;
                    }
                }
#if MANIA_USE_PLUS
                else if (globals->gameMode == MODE_ENCORE) {
                    EntityPlayer *sidekick = RSDK_GET_ENTITY(SLOT_PLAYER2, Player);
                    if (globals->stock == ID_NONE && !sidekick->classID) {
                        RSDK.SetEngineState(ENGINESTATE_FROZEN);
                        SceneInfo->timeEnabled = false;
                    }

                    if (self->camera) {
                        self->scrollDelay   = 2;
                        self->camera->state = StateMachine_None;
                    }
                }
#endif
                else {
                    RSDK.SetEngineState(ENGINESTATE_FROZEN);
                    SceneInfo->timeEnabled = false;
                    if (self->camera) {
                        self->scrollDelay   = 2;
                        self->camera->state = StateMachine_None;
                    }
                }
                break;

            case PLAYER_DEATH_DROWN:
                self->deathType = PLAYER_DEATH_NONE;
                // Bug Details:
                // setting this actually causes a slight bug, as when you die the underwater flag is cleared but your gravity strength isn't updated
                // so if you debug out, you'll have regular speed with a moon jump
                // (yes I know this is super minor but its neat to know anyways)
                self->gravityStrength = 0x1000;
                self->velocity.y      = 0;
                RSDK.PlaySfx(Water->sfxDrown, false, 255);
                self->state = Player_State_Drown;

                if (!self->sidekick) {
                    if (globals->gameMode == MODE_COMPETITION) {
                        Music_JingleFadeOut(TRACK_DROWNING, false);
                    }
#if MANIA_USE_PLUS
                    else if (globals->gameMode == MODE_ENCORE) {
                        EntityPlayer *sidekick = RSDK_GET_ENTITY(SLOT_PLAYER2, Player);
                        if (globals->stock == ID_NONE && !sidekick->classID) {
                            SceneInfo->timeEnabled = false;
                        }
                    }
#endif
                    else {
                        SceneInfo->timeEnabled = false;
                    }
                }

                if (self->camera) {
                    self->scrollDelay   = 2;
                    self->camera->state = StateMachine_None;
                }
                break;
        }
    }

    if (self->onGround) {
        if (self->nextGroundState) {
            self->state           = self->nextGroundState;
            self->nextGroundState = StateMachine_None;
            if (self->sidekick)
                RSDK_GET_ENTITY(SLOT_PLAYER1, Player)->scoreBonus = 0;
            else
                self->scoreBonus = 0;
        }

        if (self->camera) {
            if (self->animator.animationID == ANI_JUMP)
                self->camera->adjustY = self->jumpOffset;
            else
                self->camera->adjustY = 0;
        }
    }
    else if (self->nextAirState) {
        self->state        = self->nextAirState;
        self->nextAirState = StateMachine_None;
    }

    if (self->tailFrames != (uint16)-1) {
        switch (self->animator.animationID) {
            case ANI_IDLE:
            case ANI_BORED_1:
            case ANI_LOOK_UP:
            case ANI_CROUCH:
                RSDK.SetSpriteAnimation(self->tailFrames, 0, &self->tailAnimator, false, 0);
                self->tailDirection = self->direction;
                self->tailRotation  = self->rotation;
                break;

            case ANI_JUMP:
                RSDK.SetSpriteAnimation(self->tailFrames, 1, &self->tailAnimator, false, 0);
                if (self->onGround) {
                    self->tailRotation  = self->rotation;
                    self->tailDirection = self->groundVel <= 0;
                }
                else {
                    self->tailRotation = RSDK.ATan2(self->velocity.x, self->velocity.y) << 1;
                    if (self->direction == FLIP_X)
                        self->tailRotation = (self->tailRotation - 256) & 0x1FF;
                    self->tailDirection = self->direction;
                }
                break;

            case ANI_SKID:
            case ANI_PUSH:
            case ANI_HANG:
                RSDK.SetSpriteAnimation(self->tailFrames, 3, &self->tailAnimator, false, 0);
                self->tailDirection = self->direction;
                break;

            case ANI_SPINDASH:
                RSDK.SetSpriteAnimation(self->tailFrames, 2, &self->tailAnimator, false, 0);
                self->tailDirection = self->direction;
                break;

            case ANI_HANG_MOVE:
                RSDK.SetSpriteAnimation(self->tailFrames, 4, &self->tailAnimator, false, 0);
                self->tailDirection = self->direction;
                break;

            case ANI_PULLEY_HOLD:
                RSDK.SetSpriteAnimation(self->tailFrames, 5, &self->tailAnimator, false, 0);
                self->tailDirection = self->direction;
                if (self->tailDirection)
                    self->tailRotation = self->rotation + 32;
                else
                    self->tailRotation = self->rotation - 32;
                break;

            default: RSDK.SetSpriteAnimation(-1, 0, &self->tailAnimator, false, 0); break;
        }

        RSDK.ProcessAnimation(&self->tailAnimator);
    }

    RSDK.ProcessAnimation(&self->animator);
}

void Player_StaticUpdate(void)
{
#if MANIA_USE_PLUS
    // Moved here from ERZ start since flying can now be done in any stage, not just ERZ
    if (Player->superDashCooldown > 0) {
        RSDK_GET_ENTITY(SLOT_PLAYER1, Player);
        HUD_EnableRingFlash();
        --Player->superDashCooldown;
    }
#endif

    bool32 flying = false;
    bool32 tired  = false;
    if (SceneInfo->state == ENGINESTATE_REGULAR) {
        foreach_active(Player, player)
        {
            if (player->characterID == ID_TAILS) {
                int32 anim = player->animator.animationID;
                if (anim == ANI_FLY || anim == ANI_FLY_LIFT)
                    flying = true;
                if (anim == ANI_FLY_TIRED || anim == ANI_FLY_LIFT_TIRED)
                    tired = true;
            }
        }

        if (flying) {
            if (!Player->playingFlySfx) {
                RSDK.PlaySfx(Player->sfxFlying, true, 255);
                Player->playingFlySfx = true;
            }
        }

        if (tired) {
            if (!Player->playingTiredSfx) {
                RSDK.PlaySfx(Player->sfxTired, true, 255);
                Player->playingTiredSfx = true;
            }
        }
    }

    if (!flying && Player->playingFlySfx) {
        RSDK.StopSfx(Player->sfxFlying);
        Player->playingFlySfx = false;
    }

    if (!tired && Player->playingTiredSfx) {
        RSDK.StopSfx(Player->sfxTired);
        Player->playingTiredSfx = false;
    }

#if MANIA_USE_PLUS
    Player->raySwoopTimer -= 8;
    if (Player->raySwoopTimer < 0)
        Player->raySwoopTimer = 0;

    Player->rayDiveTimer -= 16;
    if (Player->rayDiveTimer < 0)
        Player->rayDiveTimer = 0;
#endif
}

void Player_Draw(void)
{
    RSDK_THIS(Player);

#if MANIA_USE_PLUS
    if (self->isGhost) {
        self->inkEffect = INK_BLEND;
        self->alpha     = 0x7F; // redundant since the ink effect is INK_BLEND, which is basically 50% alpha anyways
    }
#endif

    Entity *parent   = self->abilityPtrs[0];
    Vector2 posStore = self->position;
    if (self->state == Player_State_FlyToPlayer && parent) {
        self->position.x = parent->position.x;
        self->position.y = parent->position.y;
    }

    if (self->tailFrames != (uint16)-1) {
        int32 rotStore  = self->rotation;
        int32 dirStore  = self->direction;
        self->rotation  = self->tailRotation;
        self->direction = self->tailDirection;
        RSDK.DrawSprite(&self->tailAnimator, NULL, false);

        self->rotation  = rotStore;
        self->direction = dirStore;
    }

    // Copy the colours for any other players so stuff like super forms or etc don't effect the other players aside from P1
    // Used for sidekicks, competition players with the same player or etc
    if (self->playerID == 1 && GET_CHARACTER_ID(1) == GET_CHARACTER_ID(2) && globals->gameMode != MODE_TIMEATTACK) {
        uint32 colorStore[PLAYER_PRIMARY_COLOR_COUNT];

        switch (self->characterID) {
            case ID_SONIC:
                for (int32 c = 0; c < PLAYER_PRIMARY_COLOR_COUNT; ++c) {
                    colorStore[c] = RSDK.GetPaletteEntry(0, PLAYER_PALETTE_INDEX_SONIC + c);
                    RSDK.SetPaletteEntry(0, PLAYER_PALETTE_INDEX_SONIC + c, Player->superPalette_Sonic[c]);
                }

                RSDK.DrawSprite(&self->animator, NULL, false);

                for (int32 c = 0; c < PLAYER_PRIMARY_COLOR_COUNT; ++c) RSDK.SetPaletteEntry(0, PLAYER_PALETTE_INDEX_SONIC + c, colorStore[c]);
                break;

            case ID_TAILS:
                for (int32 c = 0; c < PLAYER_PRIMARY_COLOR_COUNT; ++c) {
                    colorStore[c] = RSDK.GetPaletteEntry(0, PLAYER_PALETTE_INDEX_TAILS + c);
                    RSDK.SetPaletteEntry(0, PLAYER_PALETTE_INDEX_TAILS + c, Player->superPalette_Tails[c]);
                }

                RSDK.DrawSprite(&self->animator, NULL, false);

                for (int32 c = 0; c < PLAYER_PRIMARY_COLOR_COUNT; ++c) RSDK.SetPaletteEntry(0, PLAYER_PALETTE_INDEX_TAILS + c, colorStore[c]);
                break;

            case ID_KNUCKLES:
                for (int32 c = 0; c < PLAYER_PRIMARY_COLOR_COUNT; ++c) {
                    colorStore[c] = RSDK.GetPaletteEntry(0, PLAYER_PALETTE_INDEX_KNUX + c);
                    RSDK.SetPaletteEntry(0, PLAYER_PALETTE_INDEX_KNUX + c, Player->superPalette_Knux[c]);
                }

                RSDK.DrawSprite(&self->animator, NULL, false);

                for (int32 c = 0; c < PLAYER_PRIMARY_COLOR_COUNT; ++c) RSDK.SetPaletteEntry(0, PLAYER_PALETTE_INDEX_KNUX + c, colorStore[c]);
                break;

#if MANIA_USE_PLUS
            case ID_MIGHTY:
                for (int32 c = 0; c < PLAYER_PRIMARY_COLOR_COUNT; ++c) {
                    colorStore[c] = RSDK.GetPaletteEntry(0, PLAYER_PALETTE_INDEX_MIGHTY + c);
                    RSDK.SetPaletteEntry(0, PLAYER_PALETTE_INDEX_MIGHTY + c, Player->superPalette_Mighty[c]);
                }

                RSDK.DrawSprite(&self->animator, NULL, false);

                for (int32 c = 0; c < PLAYER_PRIMARY_COLOR_COUNT; ++c) RSDK.SetPaletteEntry(0, PLAYER_PALETTE_INDEX_MIGHTY + c, colorStore[c]);
                break;

            case ID_RAY:
                for (int32 c = 0; c < PLAYER_PRIMARY_COLOR_COUNT; ++c) {
                    colorStore[c] = RSDK.GetPaletteEntry(0, PLAYER_PALETTE_INDEX_RAY + c);
                    RSDK.SetPaletteEntry(0, PLAYER_PALETTE_INDEX_RAY + c, Player->superPalette_Ray[c]);
                }

                RSDK.DrawSprite(&self->animator, NULL, false);

                for (int32 c = 0; c < PLAYER_PRIMARY_COLOR_COUNT; ++c) RSDK.SetPaletteEntry(0, PLAYER_PALETTE_INDEX_RAY + c, colorStore[c]);
                break;

            case ID_AMY:
                for (int32 c = 0; c < PLAYER_PRIMARY_COLOR_COUNT; ++c) {
                    colorStore[c] = RSDK.GetPaletteEntry(0, PLAYER_PALETTE_INDEX_AMY + c);
                    RSDK.SetPaletteEntry(0, PLAYER_PALETTE_INDEX_AMY + c, Player->superPalette_Amy[c]);
                }

                RSDK.DrawSprite(&self->animator, NULL, false);

                for (int32 c = 0; c < PLAYER_PRIMARY_COLOR_COUNT; ++c) RSDK.SetPaletteEntry(0, PLAYER_PALETTE_INDEX_AMY + c, colorStore[c]);
                break;
#endif
        }
    }
    else {
        // Draw the Player, no fancy stuff
        RSDK.DrawSprite(&self->animator, NULL, false);
    }

    if (self->state == Player_State_FlyToPlayer && parent) {
        self->position.x = posStore.x;
        self->position.y = posStore.y;
    }
}

void Player_Create(void *data)
{
    RSDK_THIS(Player);

    if (SceneInfo->inEditor) {
        RSDK.SetSpriteAnimation(Player->sonicFrames, ANI_IDLE, &self->animator, true, 0);
        self->characterID = ID_SONIC;
    }
    else {
        self->playerID = RSDK.GetEntitySlot(self);

        // Handle character specific stuff
        switch (self->characterID) {
            default:
            case ID_SONIC:
                self->aniFrames    = Player->sonicFrames;
                self->tailFrames   = -1;
                self->jumpOffset   = TO_FIXED(5);
                self->stateAbility = Player_JumpAbility_Sonic;
                self->sensorY      = TO_FIXED(20);

                if (globals->medalMods & MEDAL_PEELOUT) {
                    self->statePeelout = Player_Action_Peelout;
                    for (int32 f = 0; f < 4; ++f) {
                        SpriteFrame *dst = RSDK.GetFrame(self->aniFrames, ANI_DASH, f + 1);
                        SpriteFrame *src = RSDK.GetFrame(self->aniFrames, ANI_PEELOUT, f);

                        *dst = *src;
                    }
                }
                break;

            case ID_TAILS:
                self->aniFrames    = Player->tailsFrames;
                self->tailFrames   = Player->tailsTailsFrames;
                self->jumpOffset   = TO_FIXED(0);
                self->stateAbility = Player_JumpAbility_Tails;
                self->sensorY      = TO_FIXED(16);
                break;

            case ID_KNUCKLES:
                self->aniFrames    = Player->knuxFrames;
                self->tailFrames   = -1;
                self->jumpOffset   = TO_FIXED(5);
                self->stateAbility = Player_JumpAbility_Knux;
                self->sensorY      = TO_FIXED(20);
                break;

#if MANIA_USE_PLUS
            case ID_MIGHTY:
                self->aniFrames    = Player->mightyFrames;
                self->tailFrames   = -1;
                self->jumpOffset   = TO_FIXED(5);
                self->stateAbility = Player_JumpAbility_Mighty;
                self->sensorY      = TO_FIXED(20);
                break;

            case ID_RAY:
                self->aniFrames    = Player->rayFrames;
                self->tailFrames   = -1;
                self->jumpOffset   = TO_FIXED(5);
                self->stateAbility = Player_JumpAbility_Ray;
                self->sensorY      = TO_FIXED(20);
                break;

            case ID_AMY:
                self->aniFrames      = Player->amyFrames;
                self->tailFrames     = -1;
                self->jumpOffset     = TO_FIXED(5);
                self->stateAbility   = Player_JumpAbility_Amy;
                self->sensorY        = TO_FIXED(20);
                self->stateTallJump  = Player_Action_TallJump;
#if ESA_ENABLE_HAMMERWHACK
                self->stateHammerHit = Player_Action_HammerWhack;
#endif

                if (globals->medalMods & MEDAL_AMYCDR) {
                    for (int32 f = 0; f < 5; ++f) {
                        SpriteFrame *dst = RSDK.GetFrame(self->aniFrames, ANI_DASH, f);
                        SpriteFrame *src = RSDK.GetFrame(self->aniFrames, ANI_DASH_NOHAMMER, f);

                        *dst = *src;
                    }
                }
                break;
#endif
        }

        // Handle Sensor setup
        self->sensorX[0] = TO_FIXED(10);
        self->sensorX[1] = TO_FIXED(5);
        self->sensorX[2] = TO_FIXED(0);
        self->sensorX[3] = -TO_FIXED(5);
        self->sensorX[4] = -TO_FIXED(10);

        self->active         = ACTIVE_NORMAL;
        self->tileCollisions = TILECOLLISION_DOWN;
        self->visible        = true;
        self->drawGroup      = Zone->playerDrawGroup[0];
        self->scale.x        = 0x200;
        self->scale.y        = 0x200;
        self->controllerID   = self->playerID + 1;
        self->state          = Player_State_Ground;

        // Handle Input Setup
        if (!SceneInfo->entitySlot || globals->gameMode == MODE_COMPETITION) {
            self->stateInput = Player_Input_P1;
        }
#if MANIA_USE_PLUS
        else if (SceneInfo->entitySlot == 1 && globals->gameMode == MODE_TIMEATTACK) {
            StateMachine_Run(Player->configureGhostCB);
        }
#endif
        else {
            API_AssignInputSlotToDevice(self->controllerID, INPUT_AUTOASSIGN);
            self->stateInput = Player_Input_P2_AI;
            self->sidekick   = true;
        }

        AnalogStickInfoL[self->controllerID].deadzone = 0.3f;

        // Handle Sidekick Setup
        if (self->sidekick) {
            if (globals->gameMode == MODE_COMPETITION) {
                // ??
            }
            else {
                EntityPlayer *leader = RSDK_GET_ENTITY(SLOT_PLAYER1, Player);
                self->position.x     = leader->position.x - 0x200000;
                self->position.y     = leader->position.y;
                self->characterID    = GET_CHARACTER_ID(2);

                if (globals->gameMode == MODE_ENCORE) {
                    // ??
                }
                else {
                    EntityPauseMenu *pauseMenu = RSDK_GET_ENTITY(SLOT_PAUSEMENU, PauseMenu);
                    if (!RSDK.GetEntityCount(TitleCard->classID, false) && !pauseMenu->classID) {
                        RSDK.ResetEntitySlot(SLOT_PAUSEMENU, PauseMenu->classID, NULL);
                        pauseMenu->triggerPlayer = 0;
                    }
                }

                Player_HandleSidekickRespawn();
            }
        }
    }
}

void Player_Action_TallJump(void)
{
    RSDK_THIS(Player);

    self->controlLock  = 0;
    self->onGround     = true;
    self->nextAirState = 0x0;

    int32 hammeranimation = self->animator.animationID;

    if (hammeranimation != ANI_HAMMER_JUMP) {
        if (hammeranimation == ANI_HAMMER_DASH_2) {
            self->velocity.x += 1 * self->velocity.x >> 1;
        }
        else {
            if (hammeranimation == ANI_HAMMER_DASH_1)
                self->velocity.x += 1 * self->velocity.x >> 2;
        }
        self->jumpAbilityState = 1;
        self->applyJumpCap     = 1;
        self->jumpStrength     = -0x78000;

        RSDK.PlaySfx(Player->sfxJump, false, 255);
        RSDK.SetSpriteAnimation(self->aniFrames, ANI_HAMMER_JUMP, &self->animator, false, 0);

        self->state            = Player_State_TallJump;
        self->stateInput       = Player_Input_P1;
        self->onGround         = false;
        self->velocity.y       = self->jumpStrength;
        self->pushing          = 0;
        self->tileCollisions   = TILECOLLISION_DOWN;
        self->nextAirState     = Player_State_TallJump;
    }
}

// ------------------------------------
// STATES (FIXED)
// ------------------------------------

void Player_State_TallJump(void) // FIXED: Removed (EntityPlayer *self)
{
    RSDK_THIS(Player); // FIXED: Added this macro

    self->controlLock = 0;
    self->onGround    = false;

    if (self->collisionMode == CMODE_FLOOR && self->state != Player_State_Roll)
        self->position.y += self->jumpOffset;

    self->applyJumpCap = true;

    int32 jumpForce = self->gravityStrength + self->jumpStrength;

    self->velocity.x = (self->groundVel * RSDK.Cos256(self->angle) + jumpForce * RSDK.Sin256(self->angle)) >> 8;
    self->velocity.y = (self->groundVel * RSDK.Sin256(self->angle) - jumpForce * RSDK.Cos256(self->angle)) >> 8;

    if (self->controllerID < 5) {
        if (self->animator.animationID == ANI_HAMMER_JUMP) {
            if (self->animator.frameID == 4) {
                RSDK.SetSpriteAnimation(self->aniFrames, ANI_JUMP, &self->animator, false, 0);
            }
        }
    }

    if (self->velocity.y > -0x40000 && self->velocity.y < 0) {
        self->velocity.x -= self->velocity.x >> 5;
    }

    if (self->velocity.y < -0x60000) {
        self->velocity.y = -0x60000;
    }

    Player_HandleAirMovement();
}

void Player_State_HammerAttack(void) // FIXED
{
    RSDK_THIS(Player);
    
    self->groundVel = 0;
    self->velocity.x = 0;
    
    RSDK.ProcessAnimation(&self->animator);
    
    if (self->animator.frameID == 9) {
        if (self->onGround)
            self->state = Player_State_Ground;
        else
            self->state = Player_State_Air;
    }
}

void Player_State_HammerDash(void) // FIXED
{
    RSDK_THIS(Player);

    Player_HandleGroundMovement();
    RSDK.ProcessAnimation(&self->animator);

    if (self->animator.animationID == ANI_HAMMER_DASH_1) {
        if (self->animator.frameID == 3)
            RSDK.SetSpriteAnimation(self->aniFrames, ANI_HAMMER_DASH_2, &self->animator, false, 0);
    }
    
    if (!self->onGround)
        self->state = Player_State_Air;
}

void Player_State_AmyFloat(void) // FIXED
{
    RSDK_THIS(Player);

    Player_HandleAirMovement();

    if (self->onGround) {
        self->state = Player_State_Ground;
    }
    else {
        if (!self->jumpHold)
            self->state = Player_State_AmyFloat_Fall;
    }
}

void Player_State_AmyFloat_Fall(void) // FIXED
{
    RSDK_THIS(Player);
    
    Player_HandleAirMovement();

    if (self->onGround)
        self->state = Player_State_Ground;
}

void Player_State_HammerJump(void) // FIXED
{
    RSDK_THIS(Player);

    Player_HandleAirMovement();

    if (self->onGround)
        self->state = Player_State_Ground;
}

// ------------------------------------
// JUMP ABILITY
// ------------------------------------

void Player_JumpAbility_Amy(void)
{
    RSDK_THIS(Player);

    if (self->jumpPress) {
        // Double jump / Hammer logic goes here
        // If you had specific logic for Amy's double jump that called a state,
        // ensure you call it like: Player_State_Whatever(); 
    }
}

// The rest of the file (EditorDraw, etc.)
#if RETRO_INCLUDE_EDITOR
void Player_EditorDraw(void)
{
    RSDK_THIS(Player);

    RSDK.SetSpriteAnimation(Player->sonicFrames, 0, &self->animator, true, self->characterID);
    RSDK.DrawSprite(&self->animator, NULL, false);
}

void Player_EditorLoad(void)
{
    Player->sonicFrames = RSDK.LoadSpriteAnimation("Editor/PlayerIcons.bin", SCOPE_STAGE);

    RSDK_ACTIVE_VAR(Player, characterID);
    RSDK_ENUM_VAR("None", PLAYER_CHAR_NONE);
    RSDK_ENUM_VAR("Sonic", PLAYER_CHAR_SONIC);
    RSDK_ENUM_VAR("Tails", PLAYER_CHAR_TAILS);
    RSDK_ENUM_VAR("Sonic & Tails", PLAYER_CHAR_SONIC_TAILS);
    RSDK_ENUM_VAR("Knuckles", PLAYER_CHAR_KNUX);
    RSDK_ENUM_VAR("Sonic & Knuckles", PLAYER_CHAR_SONIC_KNUX);
#if MANIA_USE_PLUS
    RSDK_ENUM_VAR("Mighty", PLAYER_CHAR_MIGHTY);
    RSDK_ENUM_VAR("Sonic & Mighty", PLAYER_CHAR_SONIC_MIGHTY);
    RSDK_ENUM_VAR("Ray", PLAYER_CHAR_RAY);
    RSDK_ENUM_VAR("Sonic & Ray", PLAYER_CHAR_SONIC_RAY);
    RSDK_ENUM_VAR("Amy", PLAYER_CHAR_AMY);
    RSDK_ENUM_VAR("Sonic & Amy", PLAYER_CHAR_SONIC_AMY);
#endif

    RSDK_ACTIVE_VAR(Player, leaderID);
    RSDK_ENUM_VAR("No Override", 0);
    RSDK_ENUM_VAR("Sonic", ID_SONIC);
    RSDK_ENUM_VAR("Tails", ID_TAILS);
    RSDK_ENUM_VAR("Knuckles", ID_KNUCKLES);
#if MANIA_USE_PLUS
    RSDK_ENUM_VAR("Mighty", ID_MIGHTY);
    RSDK_ENUM_VAR("Ray", ID_RAY);
    RSDK_ENUM_VAR("Amy", ID_AMY);
#endif

    RSDK_ACTIVE_VAR(Player, sidekickID);
    RSDK_ENUM_VAR("No Override", 0);
    RSDK_ENUM_VAR("Sonic", ID_SONIC);
    RSDK_ENUM_VAR("Tails", ID_TAILS);
    RSDK_ENUM_VAR("Knuckles", ID_KNUCKLES);
#if MANIA_USE_PLUS
    RSDK_ENUM_VAR("Mighty", ID_MIGHTY);
    RSDK_ENUM_VAR("Ray", ID_RAY);
    RSDK_ENUM_VAR("Amy", ID_AMY);
#endif
}
#endif

void Player_Serialize(void)
{
    RSDK_EDITABLE_VAR(Player, VAR_UINT8, characterID);
    RSDK_EDITABLE_VAR(Player, VAR_UINT8, leaderID);
    RSDK_EDITABLE_VAR(Player, VAR_UINT8, sidekickID);
}
