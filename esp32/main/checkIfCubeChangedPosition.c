#include <stdio.h>
int checkPosition(int16_t AccelX, int16_t AccelY, int16_t AccelZ)
{

    if (AccelX < 1100 && AccelX > 900)
    {
        printf("Sensor is in position +++++X \n");
        return 1;
    }
    else if (AccelX > -1100 && AccelX < -900)
    {
        printf("Sensor is in position -----X \n");
        return 2;
    }
    else if (AccelY < 1100 && AccelY > 900)
    {
        printf("Sensor is in position +++++Y \n");
        return 3;
    }
    else if (AccelY > -1100 && AccelY < -900)
    {
        printf("Sensor is in position -----Y \n");
        return 4;
    }
    else if (AccelZ < 1100 && AccelZ > 900)
    {
        printf("Sensor is in position +++++Z \n");
        return 5;
    }
    else if (AccelZ > -1100 && AccelZ < -900)
    {
        printf("Sensor is in position -----Z \n");
        return 6;
    }

    return 0;
}