#include <project.h>

#define USBFS_DEVICE (0u)
#define USBUART_BUFFER_SIZE (64u)

int main(void)
{
    uint8 buffer[USBUART_BUFFER_SIZE];
    uint16 count;

    CyGlobalIntEnable;
    USBUART_Start(USBFS_DEVICE, USBUART_5V_OPERATION);

    for(;;)
    {
        if (USBUART_IsConfigurationChanged())
        {
            if (USBUART_GetConfiguration())
            {
                USBUART_CDC_Init();
            }
        }

        if (USBUART_GetConfiguration())
        {
            if (USBUART_DataIsReady())
            {
                count = USBUART_GetAll(buffer);

                if(count > 0)
                {
                    while(!USBUART_CDCIsReady());
                    USBUART_PutData(buffer, count);
                }
            }
        }
    }
}