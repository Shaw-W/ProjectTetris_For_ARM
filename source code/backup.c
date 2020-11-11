printf("right %d\n, idf:%d", getpid(), id);
CreateRand_R();
game_mode_R = 1;
while (1)
{
    CreateBlock_R();
    PreviewBlock_R();
    while (1)
    {
        usleep(1000000);
        do
        {
            switch (game_mode_R)
            {
            case 1:
                sharedMem[46] = 1;
                break;
            case 0:
                sharedMem[46] = 0;
                break;
            case -1:
                sharedMem[46] = -1;
            default:
                break;
            }

            //printf("start right loop\n");
            if (sharedMem[42] == 1)
            {
                printf("42\n");
                switch (sharedMem[44])
                {
                case 0:
                    if (game_mode_R == 1)
                    {
                        game_mode_R = 0;
                        printf("Player B pause the game!\n");
                    }
                    else
                    {
                        game_mode_R = 1;
                        printf("Player B restart the game!\n");
                    }
                    break;
                case 1:
                    switch (cwhich_R)
                    {
                    case 1:
                        break;
                    case 0:
                    case 5:
                    case 6:
                        if (ctype_R == 1)
                            ctype_R = 0;
                        else
                            ctype_R++;
                        if (JudgeHit_R(0))
                        {
                            ctype_R = ctype_rem_R;
                        }
                        ctype_rem_R = ctype_R;
                        break;
                    case 2:
                    case 3:
                    case 4:
                        if (ctype_R == 3)
                            ctype_R = 0;
                        else
                            ctype_R++;
                        if (JudgeHit_R(0))
                        {
                            ctype_R = ctype_rem_R;
                        }
                        ctype_rem_R = ctype_R;
                        break;
                    default:
                        printf("switch error!%d,%d,%d\n", cwhich_R, ctype_R, ctype_rem_R);
                        exit(1);
                    }
                    break;
                case 2:
                    if (!JudgeHit_R(2))
                    {
                        x_offset_R--;
                    }
                    break;
                case 3:
                    if (!JudgeHit_R(3))
                    {
                        x_offset_R++;
                    }
                    break;
                case 4:
                    if (!JudgeHit_R(1))
                    {
                        y_offset_R++;
                    }
                    break;
                default:
                    printf("big keyboard error!\n");
                    break;
                }
                printf("judge go!\n");
            }
            else
            {
                printf("42:%d\n  44:%d\n", sharedMem[42], sharedMem[44]);
            }

            //printf("end of loop");

        } while (!game_mode_R);

        if (JudgeHit_R(1))
        {
            DrawBlock_R(cwhich_R, ctype_rem_R);
            JudgeFull_R();
            break;
        }
        y_offset_R++;
        DrawBlock_R(cwhich_R, ctype_rem_R);
    }

    if (sharedMem[46] == -1)
    {
        printf("game end!");
        exit(1);
    }

    if (score_R > score && score_R == 50)
    {
        printf("B Win!\n");
        sharedMem[46] = -1;
        exit(1);
    }
}
sharedMem[46] = -1;