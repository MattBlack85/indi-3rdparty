/*
 Starlight Xpress Filter Wheel INDI Driver

 Copyright (c) 2012 Cloudmakers, s. r. o.
 All Rights Reserved.

 Code is based on SX Filter Wheel INDI Driver by Gerry Rozema
 Copyright(c) 2010 Gerry Rozema.
 All rights reserved.

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 more details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 The full GNU General Public License is included in this distribution in the
 file called LICENSE.
 */

#include "sxwheel.h"

#include "sxconfig.h"

#include <memory>
#include <unistd.h>

std::unique_ptr<SXWHEEL> sxwheel(new SXWHEEL());


SXWHEEL::SXWHEEL()
{
    FilterSlotNP[0].setMin(1);
    FilterSlotNP[0].setMax(-1);
    CurrentFilter      = 1;
    handle             = 0;
    //setDeviceName(getDefaultName());
    setVersion(VERSION_MAJOR, VERSION_MINOR);
}

SXWHEEL::~SXWHEEL()
{
    if (isSimulation())
        LOG_DEBUG("simulation: disconnected");
    else
    {
        if (handle)
            hid_close(handle);
        hid_exit();
    }
}

const char *SXWHEEL::getDefaultName()
{
    return (char *)"SX Wheel";
}

bool SXWHEEL::Connect()
{
    if (isSimulation())
    {
        LOG_DEBUG("simulation: connected");
        handle = (hid_device *)1;
        SelectFilter(CurrentFilter);
    }
    else
    {
        if (!handle)
            handle = hid_open(0x1278, 0x0920, nullptr);
        SelectFilter(CurrentFilter);
    }
    return handle != nullptr;
}

bool SXWHEEL::Disconnect()
{
    if (isSimulation())
        LOG_DEBUG("simulation: disconnected");
    else
    {
        if (handle)
            hid_close(handle);
    }
    handle = 0;
    return true;
}

bool SXWHEEL::initProperties()
{
    INDI::FilterWheel::initProperties();
    addDebugControl();
    addSimulationControl();

    MovementModeSP[UNIDIRECTIONAL].fill("UNIDIRECTIONAL", "Unidirectional", ISS_OFF);
    MovementModeSP[BIDIRECTIONAL].fill("BIDIRECTIONAL", "Bidirectional", ISS_ON);
    MovementModeSP.fill(getDeviceName(), "MOVEMENT", "Movement", OPTIONS_TAB, IP_RW, ISR_1OFMANY, 0, IPS_OK);
    defineProperty(MovementModeSP);

    return true;
}

int SXWHEEL::SendWheelMessage(int a, int b)
{
    if (isSimulation())
    {
        LOGF_DEBUG("simulation: command %d %d", a, b);
        if (a >= 0x80)
            CurrentFilter = a - 0x80;
        FilterSlotNP[0].setMax(5);
        return 0;
    }
    if (!handle)
    {
        LOG_ERROR("Filter wheel not connected.");
        return -1;
    }
    unsigned char buf[2] = { static_cast<unsigned char>(a), static_cast<unsigned char>(b) };
    int rc               = hid_write(handle, buf, 2);
    LOGF_DEBUG("SendWheelMessage: hid_write( { %d, %d } ) -> %d", buf[0], buf[1], rc);
    if (rc != 2)
    {
        LOG_ERROR("Failed to write to wheel");
        return -1;
    }
    usleep(100);
    rc = hid_read(handle, buf, 2);
    LOGF_DEBUG("SendWheelMessage: hid_read() -> { %d, %d } %d", buf[0], buf[1], rc);
    if (rc != 2)
    {
        LOG_DEBUG("Failed to read from wheel.");
        return -1;
    }
    CurrentFilter      = buf[0];
    FilterSlotNP[0].setMax(buf[1]);
    return 0;
}

int SXWHEEL::QueryFilter()
{
    SendWheelMessage(0, 0);
    return CurrentFilter;
}

bool SXWHEEL::SelectFilter(int f)
{
    TargetFilter = f;
    if(MovementModeSP[BIDIRECTIONAL].s == ISS_ON)
    {
        SendWheelMessage(f + 0x80, 0);
    }
    else
    {
        SendWheelMessage(f, 0);
    }
    SetTimer(250);
    return true;
}

void SXWHEEL::TimerHit()
{
    QueryFilter();
    if (CurrentFilter != TargetFilter)
    {
        SetTimer(250);
    }
    else
    {
        SelectFilterDone(CurrentFilter);
    }
}

bool SXWHEEL::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    // ignore if not ours
    if (!dev || strcmp(dev, getDeviceName()))
        return false;

    if (MovementModeSP.isNameMatch(name))
    {
        MovementModeSP.update(states, names, n);
        MovementModeSP.apply();
        return true;
    }
    return INDI::FilterWheel::ISNewSwitch(dev, name, states, names, n);
}

bool SXWHEEL::saveConfigItems(FILE *fp)
{
    INDI::FilterWheel::saveConfigItems(fp);

    MovementModeSP.save(fp);
    return true;
}
