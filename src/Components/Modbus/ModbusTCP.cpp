#include <Arduino.h>
#include <WiFi.h>
#include "ModbusServerWiFi.h"
#include "../../Configuration/Constants.h"
#include "../Wallbox/IWallbox.h"
#include "ModbusTCP.h"

namespace ModbusTCP
{
    ModbusServerWiFi gModbusServer;
    IWallbox *gWallbox = nullptr;

    ModbusMessage ReadInputRegister(ModbusMessage msg)
    {
        // For debugging
        Serial.println("\nMB TCP request received: READ_INPUT_REGISTER:");
        Serial.printf(" > Size: %d\n", msg.size());
        Serial.printf(" > FC: %d\n", msg.getFunctionCode());
        Serial.println(" > Data:");
        for (uint8_t i = 0; i < msg.size(); ++i)
        {
            Serial.printf("   > %d\n", msg[i]);
        }
        return ModbusMessage{};
    }

    ModbusMessage ReadHoldRegister(ModbusMessage request)
    {
        uint8_t fc = request.getFunctionCode();
        uint16_t startAddress = 0;
        uint16_t numWords = 0;
        request.get(2, startAddress);
        request.get(4, numWords);
        Serial.printf("\nModbusTCP request received: READ_HOLD_REGISTER %d words at reg. %d\n", numWords, startAddress);

        // Respond properly
        ModbusMessage response;
        uint8_t responseLengthBytes = 0;
        switch (startAddress)
        {
        case (Constants::DaheimladenRegisters::Status):
        {
            Serial.println(" -> Responding with wallbox status");
            responseLengthBytes = 2;
            const uint16_t rawState = static_cast<uint16_t>(gWallbox->GetState());
            response.add(request.getServerID(), fc, responseLengthBytes);
            response.add(rawState);
            break;
        }
        case (Constants::DaheimladenRegisters::LimitChargingCurrent):
        {
            Serial.println(" -> Responding with charging current limit");
            responseLengthBytes = 2;
            uint16_t rawCurrent = static_cast<uint16_t>(gWallbox->GetChargingCurrentLimit() * Constants::DaheimladenWallbox::CurrentFactor);
            response.add(request.getServerID(), fc, responseLengthBytes);
            response.add(rawCurrent);
            break;
        }
        case (Constants::DaheimladenRegisters::ConnectionTimeoutTime):
        {
            Serial.println(" -> Responding with connection timeout time");
            responseLengthBytes = 2;
            const uint16_t dummyValue = 60;
            response.add(request.getServerID(), fc, responseLengthBytes);
            response.add(dummyValue);
            break;
        }
        case (Constants::DaheimladenRegisters::EnergyMeter):
        {
            Serial.println(" -> Responding with energy meter value");
            responseLengthBytes = 4;
            const uint32_t rawEnergy01kWh = static_cast<uint32_t>(gWallbox->GetEnergyMeterValue() * Constants::DaheimladenWallbox::EnergyFactor);
            response.add(request.getServerID(), fc, responseLengthBytes);
            response.add(rawEnergy01kWh);
            break;
        }
        case (Constants::DaheimladenRegisters::MaxChargingCurrentAfterConnectionLoss):
        {
            Serial.println(" -> Responding with connection loss current");
            responseLengthBytes = 2;
            uint16_t rawCurrent = static_cast<uint16_t>(gWallbox->GetFailsafeCurrent() * Constants::DaheimladenWallbox::CurrentFactor);
            response.add(request.getServerID(), fc, responseLengthBytes);
            response.add(rawCurrent);
            break;
        }
        case (Constants::DaheimladenRegisters::TotalChargingPower):
        {
            Serial.println(" -> Responding with total charging power");
            responseLengthBytes = 4;
            const uint32_t powerW = static_cast<uint32_t>(gWallbox->GetChargingPower());
            response.add(request.getServerID(), fc, responseLengthBytes);
            response.add(powerW);
            break;
        }
        case (Constants::DaheimladenRegisters::ChargeCurrents):
        {
            Serial.println(" -> Responding with charging currents");
            responseLengthBytes = 12;

            float c1, c2, c3;
            gWallbox->GetChargingCurrents(c1, c2, c3);
            uint32_t rawCurrent1 = static_cast<uint32_t>(c1 * Constants::DaheimladenWallbox::CurrentFactor);
            uint32_t rawCurrent2 = static_cast<uint32_t>(c2 * Constants::DaheimladenWallbox::CurrentFactor);
            uint32_t rawCurrent3 = static_cast<uint32_t>(c3 * Constants::DaheimladenWallbox::CurrentFactor);

            response.add(request.getServerID(), fc, responseLengthBytes);
            response.add(rawCurrent1);
            response.add(rawCurrent2);
            response.add(rawCurrent3);
            break;
        }
        case (Constants::DaheimladenRegisters::ChargeVoltages):
        {
            Serial.println(" -> Responding with charging voltages");
            responseLengthBytes = 12;

            float v1, v2, v3;
            gWallbox->GetChargingVoltages(v1, v2, v3);
            uint32_t rawVoltage1 = static_cast<uint32_t>(v1 * Constants::DaheimladenWallbox::VoltageFactor);
            uint32_t rawVoltage2 = static_cast<uint32_t>(v2 * Constants::DaheimladenWallbox::VoltageFactor);
            uint32_t rawVoltage3 = static_cast<uint32_t>(v3 * Constants::DaheimladenWallbox::VoltageFactor);

            response.add(request.getServerID(), fc, responseLengthBytes);
            response.add(rawVoltage1);
            response.add(rawVoltage2);
            response.add(rawVoltage3);
            break;
        }
        case (Constants::DaheimladenRegisters::RfidStationId):
        case (Constants::DaheimladenRegisters::RfidCardId):
        {
            Serial.println(" -> Responding with ID");
            responseLengthBytes = 32;
            const uint32_t dummyValue = 0;
            response.add(request.getServerID(), fc, responseLengthBytes);
            for (uint i = 0; i < 8; ++i)
                response.add(dummyValue);
            break;
        }
        default:
        {
            Serial.println(" -> Responding with error ILLEGAL_DATA_ADDRESS");
            response.setError(request.getServerID(), fc, Modbus::Error::ILLEGAL_DATA_ADDRESS);
            break;
        }
        }

        return response;
    }

    ModbusMessage WriteHoldRegister(ModbusMessage msg)
    {
        // For debugging only
        Serial.println("\nMB TCP request received: WRITE_HOLD_REGISTER");
        Serial.printf(" > Size: %d\n", msg.size());
        Serial.printf(" > FC: %d\n", msg.getFunctionCode());
        Serial.println(" > Data:");
        for (uint8_t i = 0; i < msg.size(); ++i)
        {
            Serial.printf("   > %d\n", msg[i]);
        }
        return msg; // Echo back request
    }

    ModbusMessage WriteMultipleRegisters(ModbusMessage request)
    {
        uint8_t fc = request.getFunctionCode();
        uint16_t startAddress = 0;
        uint16_t numWords = 0;
        request.get(2, startAddress);
        request.get(4, numWords);
        Serial.printf("\nModbusTCP request received: WRITE_MULT_REGISTERS %d words to reg. %d\n", numWords, startAddress);

        // Respond properly
        switch (startAddress)
        {
        case (Constants::DaheimladenRegisters::LimitChargingCurrent):
        {
            uint16_t currentLimit = 0;
            request.get(7, currentLimit);
            Serial.printf(" -> Writing charging current limit: %d\n", currentLimit);
            gWallbox->SetChargingCurrentLimit(static_cast<float>(currentLimit * 0.1f));
            break;
        }
        default:
        {
            Serial.println(" -> Responding with error ILLEGAL_DATA_ADDRESS");
            ModbusMessage response;
            response.setError(request.getServerID(), fc, Modbus::Error::ILLEGAL_DATA_ADDRESS);
            return response;
        }
        }

        // Respond with register address and register count
        ModbusMessage response;
        response.add(request.getServerID(), fc, startAddress);
        response.add(numWords);
        return response;
    }

    void Init(IWallbox *wallbox)
    {
        Serial.println("Initializing Modbus TCP server");

        gWallbox = wallbox;

        gModbusServer.start(
            Constants::DaheimladenWallbox::TCPPort,
            Constants::DaheimladenWallbox::MaxClients,
            Constants::DaheimladenWallbox::Timeout);

        gModbusServer.registerWorker(Constants::DaheimladenWallbox::ServerId, READ_INPUT_REGISTER, ReadInputRegister);
        gModbusServer.registerWorker(Constants::DaheimladenWallbox::ServerId, READ_HOLD_REGISTER, ReadHoldRegister);
        gModbusServer.registerWorker(Constants::DaheimladenWallbox::ServerId, WRITE_HOLD_REGISTER, WriteHoldRegister);
        gModbusServer.registerWorker(Constants::DaheimladenWallbox::ServerId, WRITE_MULT_REGISTERS, WriteMultipleRegisters);
    }
};