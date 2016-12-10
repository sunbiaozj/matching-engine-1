#include <thread>
#include <vector>

#include <Logger.h>
#include <Engine_MatchingEngine.h>

#include "Gateway_Application.h"

#include "quickfix/FileStore.h"
#include "quickfix/SocketAcceptor.h"


bool term_received(false);

void sig_handler(int sig)
{
    switch (sig)
    {
        case SIGTERM:
        case SIGINT:
            term_received = true;
            signal(sig, sig_handler);
            break;
        default:
            std::cerr << sig << std::endl;
            break;
    }
}

int main( int argc, char** argv )
{
    if (argc != 2)
    {
        std::cerr << "usage: " << argv[0]
        << "CONFIG" << std::endl;
        return 1;
    }

    std::string s_matching_config = argv[1];

    auto & Logger = LoggerHolder::GetInstance();

    boost::property_tree::ptree aConfig;
    if (!boost::filesystem::exists(s_matching_config))
    {
        std::cerr << "File " << s_matching_config << " doesn't exists" << std::endl;
        return 2;
    }

    exchange::engine::MatchingEngine<> _MatchingEngine;

    boost::property_tree::ini_parser::read_ini(s_matching_config, aConfig);
    Logger.Init(aConfig);

    if( !_MatchingEngine.Configure(aConfig) )
    {
        std::cerr << "Could not configure matching engine" << std::endl;
        return 3;
    }

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

    try
    {
        FIX::SessionSettings settings( s_matching_config );

        exchange::gateway::Application application;
        FIX::FileStoreFactory storeFactory( settings );
        FIX::ScreenLogFactory logFactory( settings );
        FIX::SocketAcceptor acceptor( application, storeFactory, settings, logFactory );

        //acceptor.start();
        while ( !term_received )
        {
            _MatchingEngine.EngineListen();
            acceptor.poll();
        }
        acceptor.stop();
        return 0;
    }
    catch ( std::exception & e )
    {
        std::cout << e.what() << std::endl;
        return 4;
    }
}
