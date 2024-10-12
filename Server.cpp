#include "Server.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/asio.hpp>

// Constructor
Server::Server(const std::string& sharedFolder, const std::string& ipListFile, int port)
    : sharedFolder(sharedFolder) {
    loadAuthorizedIPs(ipListFile);
    acceptor = new boost::asio::ip::tcp::acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
}

// Destructor
Server::~Server() {
    delete acceptor;
}

// Load authorized IPs from a file
void Server::loadAuthorizedIPs(const std::string& ipListFile) {
    std::ifstream ipFile(ipListFile);
    std::string ip;
    while (std::getline(ipFile, ip)) {
        authorizedIPs.push_back(ip);
    }
    ipFile.close();
}

// Check if the client IP is authorized
bool Server::isAuthorized(const std::string& clientIP) {
    for (const auto& ip : authorizedIPs) {
        if (clientIP == ip) { // POE- may ne every ip contain "\n" charactor at last hence all conections are denied
            return true;
        }
    }
    return false; 
}

// Start the server
void Server::start() {
    std::cout << "Server started, waiting for connections..." << std::endl;

    while (true) {
        boost::asio::ip::tcp::socket* socket = new boost::asio::ip::tcp::socket(io_service);
        acceptor->accept(*socket);

        std::string clientIP = socket->remote_endpoint().address().to_string();

        if (!isAuthorized(clientIP)) {
            std::cerr << "Unauthorized access attempt from " << clientIP << std::endl;
            delete socket;
            continue;
        }

        std::cout << "Authorized connection from " << clientIP << std::endl;
        std::thread(&Server::handleClient, this, socket).detach();
    }
}

// Handle client requests
void Server::handleClient(boost::asio::ip::tcp::socket* socket) {
    try {
        char request[1024];
        std::size_t len = socket->read_some(boost::asio::buffer(request));
        std::string requestStr(request, len);

        std::istringstream iss(requestStr);
        std::string command, filename, content;
        iss >> command >> filename;

        if (command == "GET") {
            sendFileContent(socket, filename);
        } else if (command == "POST") {
            std::getline(iss, content);  // Get the remaining file content
            handleFileChange(filename, content);
            std::string clientIP = socket->remote_endpoint().address().to_string();
            broadcastChange(filename, content, clientIP);
        }
    } catch (...) {
        std::cerr << "Error handling client request." << std::endl;
    }

    delete socket;
}

// Send file content to the client
void Server::sendFileContent(boost::asio::ip::tcp::socket* socket, const std::string& filename) {
    std::string fullPath = sharedFolder + "/" + filename;
    std::ifstream file(fullPath);

    if (!file.is_open()) {
        std::cerr << "Could not open file: " << fullPath << std::endl;
        return;
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    std::string content = oss.str();

    boost::asio::write(*socket, boost::asio::buffer(content));
    file.close();
}

// Handle file changes from clients
void Server::handleFileChange(const std::string& filename, const std::string& content) {
    std::string fullPath = sharedFolder + "/" + filename;

    std::ofstream file(fullPath, std::ios::app);
    if (file.is_open()) {
        file << content;
        file.close();
    } else {
        std::cerr << "Failed to write to file: " << filename << std::endl;
    }
}

// Broadcast changes to other clients
void Server::broadcastChange(const std::string& filename, const std::string& content, const std::string& senderIP) {
    for (const auto& ip : authorizedIPs) {
        if (ip == senderIP) continue;  // Skip sender

        try {
            boost::asio::ip::tcp::socket socket(io_service);
            boost::asio::ip::tcp::resolver resolver(io_service);
            boost::asio::ip::tcp::resolver::query query(ip, "12345");
            boost::asio::connect(socket, resolver.resolve(query));

            std::string message = "UPDATE " + filename + " " + content;
            boost::asio::write(socket, boost::asio::buffer(message));
        } catch (...) {
            std::cerr << "Error broadcasting to client " << ip << std::endl;
        }
    }
}

// Main function to start the server
int main(int argc, char* argv[]) {

    std::string sharedFolder = "/home/dheeraj-6904/Main_files/server/shared_folder";
    std::string ipListFile = "/home/dheeraj-6904/Main_files/server/collaborators_ip.txt";
    int port = 8080;

    try {
        Server server(sharedFolder, ipListFile, port);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
