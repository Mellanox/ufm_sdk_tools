
#include <http_client/ClientSession.h>

/// ufm.azurehpc.core.azure-test.net has dns map in /etc/hosts 
/// ./bin/rest_tester ufm.azurehpc.core.azure-test.net 443 /ufmRest/app/ufm_version 1.1
int __main(int argc, char** argv)
{
    using namespace std::chrono_literals;

    bool enableCertificateAuth = true;

    // Check command line arguments.
    if(argc != 4 && argc != 5)
    {
        std::cerr <<
            "Usage: http-client-async <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n" <<
            "Example:\n" <<
            "    http-client-async www.example.com 80 /\n" <<
            "    http-client-async www.example.com 80 / 1.0\n";
        return EXIT_FAILURE;
    }

    auto const host = argv[1];
    auto const port = argv[2];
    auto const target = argv[3];

    // The io_context is required for all I/O
    net::io_context ioc;
    ssl::context ctx(ssl::context::tlsv12_client);

    if (enableCertificateAuth)
    {
        // Load client certificate and private key
        ctx.use_certificate_file("/tmp/certificate.crt", ssl::context::pem);
        ctx.use_private_key_file("/tmp/private-key.pem", ssl::context::pem);
    }
    else
    {
        // Disable certificate verification (insecure mode) equivilane to -k on the curl command
        ctx.set_verify_mode(ssl::verify_none);
    }

    auto client = std::make_shared<nvd::ClientSession>(ioc, ctx, host, port, nvd::AuthMethod::SSL_CERTIFICATE);

    std::optional<net::executor_work_guard<net::io_context::executor_type>> work_guard(net::make_work_guard(ioc));

    // Run the IO context in a thread pool
    std::thread t1 ([&] 
    {        
        size_t events_processed = ioc.run();
        if (events_processed == 0) {
            std::cerr << "Warning: io_context.run() exited without processing any events.\n";
        }        
    });

    client->connect();

    // Launch the asynchronous operation
    nvd::Request req;
    // nvd::AuthMethod::BASIC
    req.create(nvd::Request::HttpVerb::get, target, host);
    
    auto f = client->sendRequestAsync(req);
    auto tm = f.get();
    std::cout << "Req finished after "  << tm.count() << std::endl;
    

    // Run the I/O service. The call will return when
    // the get operation is complete.
    //ioc.run();

    work_guard.reset();
    ioc.stop();
    t1.join();

    return EXIT_SUCCESS;
}
