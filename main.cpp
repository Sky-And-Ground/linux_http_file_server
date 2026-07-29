#include "global_terminator.h"
#include "global_config.h"
#include "async_logger.h"
#include "server.h"
#include "utils.h"
#include "reply.h"
#include "filesystem_ops.h"
#include <algorithm>
#include <vector>
#include <array>
#include <exception>
#include <unistd.h>
#include <cstdio>
#include <cerrno>
#include <cstring>

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

std::string concatenate_path(const std::string& left, const std::string& right) {
    if (left.empty()) {
        return right;
    }
    else if (right.empty()) {
        return left;
    }

    std::string result = left;

    bool left_ends_with_slash = (left.back() == '/');
    bool right_starts_with_slash = (right.front() == '/');

    if (left_ends_with_slash) {
        if (right_starts_with_slash) {
            result.pop_back();
            result += right;
        }
        else {
            result += right;
        }
    }
    else {
        if (right_starts_with_slash) {
            result += right;
        }
        else {
            result += "/";
            result += right;
        }
    }

    return result;
}

std::string file_size_to_str(double fileSize, int precision) {
    int level = 0;
    std::string str;

    while (true) {
        if (fileSize / 1024.0 < 1.0f || level == 4) {
            break;
        }
        else {
            fileSize /= 1024.0;
            ++level;
        }
    }

    str = utils::double_to_str(fileSize, precision);

    switch (level) {
    case 0:
        str += " B";
        break;
    case 1:
        str += " KB";
        break;
    case 2:
        str += " MB";
        break;
    case 3:
        str += " GB";
        break;
    case 4:
    default:
        str += " TB";
        break;
    }

    return str;
}

std::string build_dir_list(const std::string& path, const std::string& url) {
    std::string temp;
    temp.reserve(65536);

    temp += R"(
<html>
    <head>
        <h1>Http File Server</h1>
    </head>
    <body>
        <form id="uploadForm" action="http://)";
    temp += GlobalConfig::instance().get_ip() + ":" + std::to_string(GlobalConfig::instance().get_port());
    temp += url;

    temp += R"(
" method="POST" enctype="multipart/form-data"><input type="file" id="fileInput" name="fileInput" /><button type="submit">Upload</button>
        </form>
        <hr>
            <ul>
)";

    // just sort the names, then the web page content would be better.
    std::vector<std::string> names = fs_ops::walk_dir(path);
    std::sort(names.begin(), names.end());

    for (const auto& name : names) {
        std::string completePath = concatenate_path(path, name);
        std::string completeUrl = concatenate_path(url, name);

        std::string line;
        line.reserve(256);

        line += "<li><a href=\"";
        line += completeUrl;
        line += "\">";

        line += name;

        if (fs_ops::is_dir(completePath)) {
            line += "/";
        }

        if (fs_ops::is_regular_file(completePath)) {
            line += "</a><span>&nbsp;&nbsp;";

            size_t fileSize;            
            if (fs_ops::file_size(completePath, fileSize)) {
                line += file_size_to_str((double)fileSize, 2);
            }

            line += "</span></li>\n";
        }
        else {
            line += "</a></li>\n";
        }

        temp += line;
    };

    temp += "</ul><hr></body></html>";
    return temp;
}

void send_dir_list_data(int fd, const std::string& completePath, const std::string& basePath) {
    std::string dirList = build_dir_list(completePath, basePath);

    Response response;
    response.http_version_major = 1;
    response.http_version_minor = 1;
    response.code = HttpStatusCode::ok;
    response.message = http_status_code_str(response.code);

    response.headers.emplace("Connection", "closed");
    response.headers.emplace("Content-Type", "text/html");
    response.headers.emplace("Content-Length", std::to_string(dirList.length()));

    send_response(fd, response);
    send_all(fd, dirList);
}

bool check_utf8_bom(const char* buf) {
    return (unsigned char)buf[0] == 0xEF 
        && (unsigned char)buf[1] == 0xBB 
        && (unsigned char)buf[2] == 0xBF;
}

void send_file_data(int fd, const std::string& completePath) {
    Response response;
    response.http_version_major = 1;
    response.http_version_minor = 1;
    response.code = HttpStatusCode::ok;
    response.message = http_status_code_str(response.code);

    std::string extension = fs_ops::get_file_extension(completePath);
    response.headers.emplace("Content-Type", GlobalConfig::instance().extension_to_mime(extension));
    response.headers.emplace("Transfer-Encoding", "chunked");
    response.headers.emplace("Connection", "closed");

    if (send_response(fd, response) <= 0) {
        LOG_ERROR("send chunked data response failed\n");
        return;
    }

    // let the file data be sent by chunked.
    std::array<char, 8192> buf;

    int file_fd = open(completePath.c_str(), O_RDONLY);
    if (file_fd < 0) {
        return;
    }

    ssize_t read_bytes = 0;
    int ret;

    while ((read_bytes = read(file_fd, buf.data(), buf.size())) > 0) {
        /* 
            some files would be start with a utf8 BOM, but not every web client could parse
            that normally, so just skip it.
        */
        if (read_bytes > 3 && check_utf8_bom(buf.data())) {
            LOG_WARN("%s: found the BOM\n", completePath.c_str());
            ret = send_chunked_data(fd, buf.data() + 3, (size_t)(read_bytes - 3));
        }
        else {
            ret = send_chunked_data(fd, buf.data(), (size_t)read_bytes);
        }

        if (ret < 0) {
            LOG_ERROR("send chunked data failed, %d, %s\n", errno, strerror(errno));
            close(file_fd);
            return;
        }
        else if (ret == 0) {
            LOG_ERROR("send chunked data failed, remote closed\n");
            close(file_fd);
            return;
        }
    }

    if (read_bytes < 0) {
        LOG_ERROR("send chunked data failed, bad read, %d, %s\n", errno, strerror(errno));
        close(file_fd);
        return;
    }

    close(file_fd);
    send_chunked_end_flag(fd);
}

void serve_file_or_dir_list(int fd, const Request& req) {
    // this path would be used in the callback later, so we just uses the shared_ptr to expand its life time.
    std::string completePath = concatenate_path(GlobalConfig::instance().get_root_path(), req.url);

    if (fs_ops::is_dir(completePath)) {
        send_dir_list_data(fd, completePath, req.url);
    }
    else if (fs_ops::is_regular_file(completePath)) {
        send_file_data(fd, completePath);
    }
    else {
        send_template_html_reply(fd, 1, 1, HttpStatusCode::not_found);
    }
}

std::string build_upload_success_page(const std::string& urlPath) {
    std::string result;
    result.reserve(1024);

    result += R"(
<!DOCTYPE html>
<html>
<head>
    <title>upload success</title>
</head>
<body>
    <p>upload success<button onclick="window.location.href=')";

    result += urlPath;

    result += R"('">go back</button></p>
</body>
</html>
)";

    return result;
}

void handle_upload_files(int fd, const Request& req) {
    if (req.is_multipart_formdata) {
        for (const MultipartPart& multipart : req.multipart_formdata) {
            if (!multipart.filename.empty()) {
                std::string completePath = concatenate_path(GlobalConfig::instance().get_root_path(), req.url);
                std::string saveFilePath = concatenate_path(completePath, multipart.filename);

                std::ofstream out { saveFilePath, std::ios::binary };
                out << multipart.data;
            }
        }
    }

    send_template_html_reply(fd, 1, 1, HttpStatusCode::ok);
}

void http_handler(int fd, const char* ip, int port, const Request& req) {
    LOG_INFO("%s:%d %s %s\n", ip, port, req.method.c_str(), req.url.c_str());

    if (req.method == "GET") {
        serve_file_or_dir_list(fd, req);
    }
    else if (req.method == "POST") {
        handle_upload_files(fd, req);
    }
    else {
        send_template_html_reply(fd, 1, 1, HttpStatusCode::method_not_allowed);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <ip> <port> <root_dir>\n", argv[0]);
        return -1;
    }

    int port = utils::parse_port(argv[2]);
    if (port < 0) {
        fprintf(stderr, "given port is invalid: %s\n", argv[2]);
        return -1;
    }

    if (!GlobalConfig::instance().load_mime("mime.txt")) {
        fprintf(stderr, "please make sure that you have the mime.txt, and it satisfies the required format\n");
        return -1;
    }

    /*
        this program just uses some socket functions like send, if the remote peer is closed, the default behaviour for the operating
        system is sending a SIGPIPE signal to the program, then the program would exit, that would be not accecptable, so here
        I just ignore that action, then I can check the returned value of send() call.
    */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        fprintf(stderr, "signal call failed, %d, %s\n", errno, strerror(errno));
        return -1;
    }

    if (daemon(1, 0) < 0) {
        fprintf(stderr, "become daemon failed, %d, %s\n", errno, strerror(errno));
        return -1;
    }
    
    async_log::Logger::instance().open_file("hfs.log");
    async_log::Logger::instance().set_level(async_log::Level::debug);

    std::set_terminate(global_terminate_handler);

    GlobalConfig::instance().set_ip(argv[1]);
    GlobalConfig::instance().set_port(port);
    GlobalConfig::instance().set_root_path(argv[3]);

    Server server;
    server.set_http_method_callback(http_handler);
    server.start();
    return 0;
}
