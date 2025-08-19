#pragma once

#include "access_coordinator.cpp"
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <ctime>

#define FAC_VERSION "1.0"


//defined in main.cpp and main_wingui.cpp
bool load_config();
bool save_config();
bool load_config(std::string& filepath, std::string& user, std::string& sshHostname, std::string& sshUsername, std::string& sshPassword, std::string& remoteBaseDir);

void print_time()
{
  auto now = std::chrono::system_clock::now();
  std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm local_tm = *std::localtime(&now_time);
  std::cout << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S") << '\n';
}

void print_current_context(AccessCoordinator& coordinator)
{
  log_info("Current user: ", coordinator.mUser.get_data(), "\n");
  log_info("Current file: ", coordinator.mFullLocalPath.get_data(), "\n");
}

int evaluate_expression(char const* expression, AccessCoordinator& coordinator)
{
  if (string_begins_with(expression, "showbackups"))
  {
    coordinator.show_backups();
  }
  else if (string_begins_with(expression, "show"))
  {
    print_current_context(coordinator);
  }
  else if (string_begins_with(expression, "version"))
  {
    log_info(FAC_VERSION);
  }
  else if (string_begins_with(expression, "download"))
  {
    print_time();
    coordinator.download();
  }
  else if (string_begins_with(expression, "upload"))
  {
    print_time();
    coordinator.upload();
  }
  else if (string_begins_with(expression, "rollback"))
  {
    std::string input;
    {
      std::cout << "How many versions back do you want to go? (0 to 4): ";
      std::getline(std::cin, input);
    }
    if (!input.empty() && std::all_of(input.begin(), input.end(), ::isdigit))
    {
      int stepsBack = std::clamp(std::stoi(input), 0, 4);
      coordinator.rollback(stepsBack);
    }
    else
    {
      log_info("Aborting rollback.");
    }
  }
  else if (string_begins_with(expression, "reserve"))
  {
    print_time();
    coordinator.reserve();
  }
  else if (string_begins_with(expression, "release"))
  {
    print_time();
    coordinator.release(false);
  }
  else if (string_begins_with(expression, "forcerelease"))
  {
    print_time();
    coordinator.release(true);
  }
  else if (string_begins_with(expression, "user"))
  {
    std::string input;
    if (string_begins_with(expression, "user="))
    {
      input = expression + 5;
      if (input.size() >= 2 && input.front() == '"' && input.back() == '"')
      {
        input = input.substr(1, input.size() - 2);
      }
    }
    else
    {
      std::cout << "Set current context user: ";
      std::getline(std::cin, input);
    }
    std::cout << "Setting user to: " << input << '\n';
    coordinator.mUser = input;
  }
  else if (string_begins_with(expression, "file"))
  {
    std::string input;
    if (string_begins_with(expression, "file="))
    {
      input = expression + 5;
      if (input.size() >= 2 && input.front() == '"' && input.back() == '"')
      {
        input = input.substr(1, input.size() - 2);
      }
    }
    else
    {
      std::cout << "Set current context file: ";
      std::getline(std::cin, input);
    }
    std::cout << "Setting file to: " << input << '\n';
    coordinator.set_filepath(input);
  }
  else if (string_begins_with(expression, "loadconfig"))
  {
    std::string filepath;
    std::string user;
    std::string sshHostname;
    std::string sshUsername;
    std::string sshPassword;
    std::string remoteBaseDir;
    if (load_config(filepath, user, sshHostname, sshUsername, sshPassword, remoteBaseDir))
    {
      coordinator.init(filepath, user, sshHostname, sshUsername, sshPassword, remoteBaseDir);
      std::cout << "Loaded access.config from disk.\n";
      print_current_context(coordinator);
    }
  }
  else if (string_begins_with(expression, "saveconfig"))
  {
    if (save_config())
    {
      std::cout << "Saved cached configuration.\n";
      print_current_context(coordinator);
    }
  }
  else if (string_begins_with(expression, "exit"))
  {
    return false;
  }
  else if (string_begins_with(expression, "agreeall"))
  {
    coordinator.mAgreeAllPrompts = true;
  }
  else
  {
    std::cout << "Command unknown, available expressions are:\n"
             "----------------------------------------------\n"
             "--- Primary File Commands --------------------\n"
             "download -> Downloads the configured file from the server, overwriting your local version.\n"
             "upload -> Uploads your local version of the file to the server, overwriting the remote version.\n"
             "----------------------------------------------\n"
             "--- Secondary File Commands ------------------\n"
             "reserve -> Explicitly reserve the remote file for your current user and IP without otherwise manipulating files.\n"
             "release -> Explicitly release the remote file so other people can access it without otherwise manipulating files.\n"
             "forcerelease -> Force release the remote file, no matter who currently has it reserved.\n"
             "rollback -> Overwrites the main file with one of the available backup files.\n"
             "showbackups -> Lists currently available backups.\n"
             "----------------------------------------------\n"
             "--- Local & Config Commands ------------------\n"
             "version -> Shows current version of the fac.\n"
             "show -> Shows current context user and filepath.\n"
             "user -> Set the current context user (user=\"example\" to input the prompt directly).\n"
             "file -> Set the current context filepath (file=\"example filepath\" to input the prompt directly).\n"
             "loadconfig -> Reloads configuration from access.config, useful if you made manual changes to access.config.\n"
             "saveconfig -> Save the current configuration to access.config.\n"
             "exit -> Exits the application.\n"
             "agreeall -> Confirms all file related prompts without asking, place as first argument if gRunning from command line.\n";
  }
  return true;
}

