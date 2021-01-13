#include "stdafx.h"
#include <strstream>

#include "QuestPenalizationExecuter.h"
#include "QuestServer.h"

CQuestPenalizationExecuter::CQuestPenalizationExecuter(const std::string& ip, int port, const std::string& usr, const std::string& psw,
                                                       int id, int request_threshold, const std::string& name) : 
                                                       CRemoteDevice(ip, port, usr, psw, id, request_threshold, name), main_server(NULL)
{
}

CQuestPenalizationExecuter::~CQuestPenalizationExecuter()
{
}

ERemoteDeviceResponse CQuestPenalizationExecuter::Check_internal() const
{
    return ( (main_server != NULL) ? TRUE_RESPONSE : FALSE_RESPONSE );
}

ERemoteDeviceResponse CQuestPenalizationExecuter::Activate_internal() const
{
    return ( (main_server != NULL) ? TRUE_RESPONSE : FALSE_RESPONSE );
}

ERemoteDeviceResponse CQuestPenalizationExecuter::DeActivate_internal() const
{
    return ( (main_server != NULL) ? TRUE_RESPONSE : FALSE_RESPONSE );
}

ERemoteDeviceResponse CQuestPenalizationExecuter::IsActivated_internal() const
{
    return ( (main_server != NULL) ? TRUE_RESPONSE : FALSE_RESPONSE );
}

enum EPenalizeType
{
    TIME = 0,
    DEFEAT = 1,
    VICTORY = 2,
    PENALIZATION_BOUND = 3
};

const static char TIME_SENSOR_PENALIZATION[] = "TIME";
const static char DEFEAT_SENSOR_PENALIZATION[] = "DEFEAT";
const static char VICTORY_SENSOR_PENALIZATION[] = "VICTORY";

static EPenalizeType GetSensorPenalizationType(const char* penalization_str)
{
    EPenalizeType ret = TIME;

    if ( strcmp(penalization_str, TIME_SENSOR_PENALIZATION) == 0 ) ret = TIME;
    else if ( strcmp(penalization_str, DEFEAT_SENSOR_PENALIZATION) == 0 ) ret = DEFEAT;
    else if ( strcmp(penalization_str, VICTORY_SENSOR_PENALIZATION) == 0 ) ret = VICTORY;

    return ret;
}

ERemoteDeviceResponse CQuestPenalizationExecuter::DoAction_internal(const void* param/* = NULL*/) const
{
    ERemoteDeviceResponse ret = FALSE_RESPONSE;

    if ( (main_server != NULL) && (param != NULL) )
    {
        const char* param_str = (const char*)param;
        std::istrstream penalizations_stream(param_str);

        char penalization_buffer[30];
        penalizations_stream >> penalization_buffer;

        EPenalizeType penalize_type = GetSensorPenalizationType(penalization_buffer);

        if ( penalize_type != PENALIZATION_BOUND )
        {
            double penalization;
            penalizations_stream >> penalization;

            if ( penalization != 0. )
            {
                char msg_buffer[1024];

                if ( penalize_type == TIME )
                {
                    main_server->IncrementTimePenalization(int(penalization));

                    if ( main_server->GetLogger() )
                    {
                        if ( penalization > 0 )
                            main_server->GetLogger()->LogNumberWithPattern(int(penalization), "Активирован временной штраф: %d (ms)");

                        if ( penalization < 0)
                            main_server->GetLogger()->LogNumberWithPattern(int(-penalization), "Активировано временное вознаграждение: %d (ms)");
                    }
                }
                else
                {
                    if ( (penalize_type == DEFEAT) && (penalization > 0) )
                    {
                        main_server->IncrementDefeatPenalization(penalization);

                        if ( main_server->GetLogger() )
                        {
                            sprintf(msg_buffer, "Активирован процент поражения %f, общий процент %f", penalization, main_server->GetDefeatPenalization());
                            main_server->GetLogger()->LogMessage(msg_buffer);
                        }
                    }
                    else
                    {
                        if ( (penalize_type == VICTORY) && (penalization > 0) )
                        {
                            main_server->IncrementVictoryPenalization(penalization);

                            if ( main_server->GetLogger() )
                            {
                                sprintf(msg_buffer, "Активирован процент победы %f, общий процент %f", penalization, main_server->GetVictoryPenalization());
                                main_server->GetLogger()->LogMessage(msg_buffer);
                            }
                        }
                    }
                }
            }

            ret = TRUE_RESPONSE;
        }
    }

    return ret;
}


