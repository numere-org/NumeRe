/* GRG: Added a lot of GTelnet:: prefixes to correctly form pointers
 *   to functions in all the tables. Example:
 *   &telnet_iac -> &GTelnet::telnet_iac
 */

// Copyright (c) Derry Bryson, 1999

#ifdef __GNUG__
#pragma implementation "gtelnet.hpp"
#endif

#include "gterm.hpp"
#include "gtelnet.hpp"


GTelnet::StateOption GTelnet::telnet_normal_state[] =
{
    { IAC,         &GTelnet::telnet_iac,         telnet_cmd_state },
    { -1,          0,                   telnet_normal_state }
};

GTelnet::StateOption GTelnet::telnet_cmd_state[] =
{
    { IAC,         &GTelnet::telnet_binary_iac,  telnet_normal_state },
    { NOP,         &GTelnet::telnet_eat,         telnet_normal_state },
    { DATAMARK,    &GTelnet::telnet_eat,         telnet_normal_state },
    { BREAK,       &GTelnet::telnet_eat,         telnet_normal_state },
    { IP,          &GTelnet::telnet_eat,         telnet_normal_state },
    { AO,          &GTelnet::telnet_eat,         telnet_normal_state },
    { AYT,         &GTelnet::telnet_eat,         telnet_normal_state },
    { EC,          &GTelnet::telnet_eat,         telnet_normal_state },
    { EL,          &GTelnet::telnet_eat,         telnet_normal_state },
    { GA,          &GTelnet::telnet_eat,         telnet_normal_state },
    { SB,          &GTelnet::telnet_eat,         telnet_sub_state },
    { WILL,        &GTelnet::telnet_cmd,         telnet_will_state },
    { WONT,        &GTelnet::telnet_cmd,         telnet_normal_state },
    { DO,          &GTelnet::telnet_cmd,         telnet_do_state },
    { DONT,        &GTelnet::telnet_cmd,         telnet_dont_state },
    { -1,          &GTelnet::telnet_eat,         telnet_normal_state }
};

GTelnet::StateOption GTelnet::telnet_sub_state[] =
{
    { SE,          &GTelnet::telnet_eat,         telnet_normal_state },
    { -1,          &GTelnet::telnet_eat,         telnet_sub_state }
};

GTelnet::StateOption GTelnet::telnet_will_state[] =
{
    { TRANSMIT_BINARY,      &GTelnet::telnet_will,            telnet_normal_state },
    { -1,                   &GTelnet::telnet_eat,             telnet_normal_state }
};

GTelnet::StateOption GTelnet::telnet_do_state[] =
{
    { TRANSMIT_BINARY,      &GTelnet::telnet_do,              telnet_normal_state },
    { TERMINAL_TYPE,        &GTelnet::telnet_do,              telnet_normal_state },
    { -1,                   &GTelnet::telnet_wont,            telnet_normal_state }
};

GTelnet::StateOption GTelnet::telnet_dont_state[] =
{
    { TRANSMIT_BINARY,      &GTelnet::telnet_dont,            telnet_normal_state },
    { TERMINAL_TYPE,        &GTelnet::telnet_dont,            telnet_normal_state },
    { -1,                   &GTelnet::telnet_dont,            telnet_normal_state }
};

GTelnet::GTelnet(int w, int h) : GTerm(w, h)
{
    telnet_binary_recv = 0;
    telnet_binary_send = 0;
    telnet_termid = strdup("vt100");
    telnet_current_state = telnet_normal_state;
}

GTelnet::~GTelnet()
{
    if(telnet_termid)
        free(telnet_termid);
}

void
GTelnet::ProcessInput(int len, unsigned char *data)
{
//printf("GTelnet::ProcessInput called...\n");
    int
    deferUpdate,
    i;

    StateOption
    *last_state;

    deferUpdate = GetMode() & DEFERUPDATE;
    SetMode(GetMode() | DEFERUPDATE);

    telnet_input_data = data;

    while(len)
    {
//printf("GTelnet::ProcessInput(): processing %d, mode = %x...\n", *telnet_input_data, GetMode());
        telnet_process_data = 1;
        for(i = 0;
                telnet_current_state[i].byte != -1 &&
                telnet_current_state[i].byte != *telnet_input_data; i++)
            ;
        last_state = telnet_current_state + i;
        telnet_current_state = last_state->next_state;
        if(last_state->action)
            (this->*(last_state->action))();
        if(telnet_process_data)
            GTerm::ProcessInput(1, string((char*)telnet_input_data));
        telnet_input_data++;
        len--;
    }
    if(!deferUpdate)
    {
        SetMode(GetMode() & ~DEFERUPDATE);
        Update();
    }
//printf("GTelnet::ProcessInput(): mode = %x\n", GetMode());
}

void
GTelnet::ProcessOutput(int len, unsigned char *data)
{
    while(len)
    {
        if(telnet_binary_send && *data == 255)
            GTerm::ProcessOutput(1, data);
        GTerm::ProcessOutput(1, data);
        data++;
        len--;
    }
}

void
GTelnet::ModeChange(int state)
{
    if(state & PC)
    {
        SendWill(TRANSMIT_BINARY);
        SendDo(TRANSMIT_BINARY);
    }
    else
    {
        SendWont(TRANSMIT_BINARY);
        SendDont(TRANSMIT_BINARY);
    }
    GTerm::ModeChange(state);
}

void
GTelnet::Reset()
{
    telnet_binary_recv = 0;
    telnet_binary_send = 0;
    telnet_current_state = telnet_normal_state;
    GTerm::Reset();
}

void
GTelnet::SetTermID(char *termId)
{
    if(telnet_termid)
        free(telnet_termid);

    telnet_termid = strdup(termId);
}

void
GTelnet::telnet_iac(void)
{
//printf("telnet_iac() called...\n");
    telnet_process_data = 0;
//printf("telnet_iac() exited...\n");
}

void
GTelnet::telnet_binary_iac(void)
{
//printf("telnet_binary_iac() called...\n");
    if(!telnet_binary_recv)
        telnet_process_data = 0;
}

void
GTelnet::telnet_eat(void)
{
//printf("telnet_eat() called...\n");
    telnet_process_data = 0;
}

void
GTelnet::telnet_cmd(void)
{
//printf("telnet_cmd() called with %d...\n", *telnet_input_data);
    telnet_lastcmd = *telnet_input_data;
    telnet_process_data = 0;
//printf("telnet_cmd() exited...\n");
}

void
GTelnet::telnet_do(void)
{
    unsigned char
    buf[4];

//printf("GTelnet::telnet_do(): ");
    switch(*telnet_input_data)
    {
        case TERMINAL_TYPE :
//printf("sending terminal type...\n");
            buf[0] = IAC;
            buf[1] = WILL;
            buf[2] = TERMINAL_TYPE;
            ProcessOutput(3, buf);
            buf[0] = IAC;
            buf[1] = SB;
            buf[2] = TERMINAL_TYPE;
            buf[3] = 0;
            ProcessOutput(4, buf);
            ProcessOutput(strlen(telnet_termid), (unsigned char *)telnet_termid);
            buf[0] = IAC;
            buf[1] = SE;
            ProcessOutput(2, buf);
            break;

        case TRANSMIT_BINARY :
//printf("enabling binary recv mode...\n");
            telnet_binary_recv = 1;
            break;
    }
    telnet_process_data = 0;
}

void
GTelnet::telnet_will(void)
{
//printf("GTelnet::telnet_will(): ");
    switch(*telnet_input_data)
    {
        case TRANSMIT_BINARY :
//printf("sending do binary...\n");
            if(!telnet_binary_recv)
                SendWill(TRANSMIT_BINARY);
            if(!telnet_binary_send)
            {
                SendDo(TRANSMIT_BINARY);
                telnet_binary_send = 1;
            }
            break;
    }
    telnet_process_data = 0;
}

void
GTelnet::telnet_dont(void)
{
    switch(*telnet_input_data)
    {
        case TRANSMIT_BINARY :
            telnet_binary_recv = 0;
            break;
    }
    telnet_process_data = 0;
}

void
GTelnet::telnet_wont(void)
{
//printf("telnet_wont() called (%d)...\n", *telnet_input_data);
    SendWont(*telnet_input_data);
    telnet_process_data = 0;
//printf("telnet_wont() exited...\n");
}

void
GTelnet::SendDo(int cmd)
{
    unsigned char
    buf[3];

    buf[0] = IAC;
    buf[1] = DO;
    buf[2] = cmd;
    SendBack(3, (char *)buf);
}

void
GTelnet::SendWill(int cmd)
{
    unsigned char
    buf[3];

    buf[0] = IAC;
    buf[1] = WILL;
    buf[2] = cmd;
    SendBack(3, (char *)buf);
}

void
GTelnet::SendDont(int cmd)
{
    unsigned char
    buf[3];

    buf[0] = IAC;
    buf[1] = DONT;
    buf[2] = cmd;
    SendBack(3, (char *)buf);
}

void
GTelnet::SendWont(int cmd)
{
    unsigned char
    buf[3];

    buf[0] = IAC;
    buf[1] = WONT;
    buf[2] = cmd;
    SendBack(3, (char *)buf);
}



