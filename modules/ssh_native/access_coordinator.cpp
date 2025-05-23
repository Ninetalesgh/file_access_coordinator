#include "access_coordinator.h"
#include "debug.h"
#include "string_format.h"

#include <iostream>
#include <chrono>

#ifndef S_IRUSR
#define	S_IRUSR	0400
#define	S_IWUSR	0200
#endif

void AccessCoordinator::_bind_methods()
{
#ifndef NO_GODOT
  ClassDB::bind_method(D_METHOD("init", "filepath", "user", "sshUsername", "sshHostname", "sshPassword", "remoteBaseDir"), &AccessCoordinator::init);
  ClassDB::bind_method(D_METHOD("reserve"), &AccessCoordinator::reserve);
  ClassDB::bind_method(D_METHOD("download"), &AccessCoordinator::download);
  ClassDB::bind_method(D_METHOD("upload"), &AccessCoordinator::upload);
  ClassDB::bind_method(D_METHOD("release", "overridePermission"), &AccessCoordinator::release);
  ClassDB::bind_method(D_METHOD("fetch_output"), &AccessCoordinator::fetch_output);
#endif
}

String AccessCoordinator::fetch_output()
{
  #if defined(FAC_WINGUI)
  char buffer[BSE_STACK_BUFFER_SMALL];
  buffer[0] = '\0';
  string_replace_char(buffer, sizeof(buffer), output.get_data(), '\n', "\r\n");
  String result = buffer;
  #else
  String result = output;
  #endif
  output = "";
  return result;
}

bool AccessCoordinator::reserve()
{
  if (!mSession)
  {
    log_error("Error: SSH session not initialized, call init first.");
    return false;
  }

  return SSH_OK == reserve_remote_file_for_local_user(mRemoteBaseDir.utf8().get_data(), mFilename.utf8().get_data(), mUser.utf8().get_data(), mIpAddress);
}

bool AccessCoordinator::download()
{
  if (!mSession)
  {
    log_error("Error: SSH session not initialized, call init first.");
    return false;
  }

  if(SSH_OK != reserve_remote_file_for_local_user(mRemoteBaseDir.utf8().get_data(), mFilename.utf8().get_data(), mUser.utf8().get_data(), mIpAddress))
  {
    return false; 
  }

  char stringFormatBuffer[BSE_STACK_BUFFER_LARGE];
  stringFormatBuffer[0] = '\0';

  string_format(stringFormatBuffer, sizeof(stringFormatBuffer), "Confirm overwrite of local file '", mFullLocalPath.utf8().get_data(), "'?");
  show_confirmation_dialog("Initiating Download", stringFormatBuffer, &AccessCoordinator::on_download_dialog_confirm);

  return true;
}

bool AccessCoordinator::upload()
{
  if (!mSession)
  {
    log_error("Error: SSH session not initialized, call init first.");
    return false;
  }

  char stringFormatBuffer[BSE_STACK_BUFFER_LARGE];
  stringFormatBuffer[0] = '\0';

  if (mReservedFileCache.find(mFilename) != -1)
  {
    on_upload_dialog_confirm();
  }
  else
  {
    string_format(stringFormatBuffer, sizeof(stringFormatBuffer), "You did not reserve '", mFilename.utf8().get_data(), "' this session,\n are you sure you want to attempt to overwrite the file on the server?");
    show_confirmation_dialog("Initiating Upload", stringFormatBuffer, &AccessCoordinator::on_upload_dialog_confirm);
  }

  return true;
}

bool AccessCoordinator::release(bool overridePermission)
{
  if (!mSession)
  {
    log_error("SSH session not initialized, call init first.");
    return false;
  }

  char stringFormatBuffer[BSE_STACK_BUFFER_LARGE];
  stringFormatBuffer[0] = '\0';

  if (overridePermission)
  {
    string_format(stringFormatBuffer, sizeof(stringFormatBuffer), "Force release of remote file '", mFilename.utf8().get_data(), "'?\nThis will overwrite any existing reservation.");
    show_confirmation_dialog("Force Release", stringFormatBuffer, &AccessCoordinator::on_force_release_dialog_confirm);
    return true;
  }
  else
  {
    return SSH_OK == release_remote_file_from_local_user(mRemoteBaseDir.utf8().get_data(), mFilename.utf8().get_data(), mUser.utf8().get_data(), mIpAddress, false);
  }
}

ReserveState AccessCoordinator::get_reserve_state(String* outOwner, s64* outFileSize)
{
  return get_reserve_state_of_remote_file(outOwner, outFileSize, mRemoteBaseDir.utf8().get_data(), mFilename.utf8().get_data(), mUser.utf8().get_data(), mIpAddress);
}

bool AccessCoordinator::set_filepath(String filepath)
{
  mFullLocalPath = filepath;
  mFilename = mFullLocalPath.get_file();
  char fullRemotePath[BSE_STACK_BUFFER_SMALL];
  string_format(fullRemotePath, sizeof(fullRemotePath), mRemoteBaseDir.utf8().get_data(), "/", mFilename.utf8().get_data());
  mFullRemotePath = fullRemotePath;
  return true;
}

bool AccessCoordinator::init(String filepath, String user, String sshHostname, String sshUsername, String sshPassword, String remoteBaseDir)
{ 
  if (mSession)
  {
    shutdown_session();
  }
  
  mFullLocalPath = filepath;
  mFilename = mFullLocalPath.get_file();

  mUser = user;
  mSshUsername = sshUsername;
  mSshHostname = sshHostname;
  mSshPassword = sshPassword;
  mRemoteBaseDir = remoteBaseDir;
  
  char fullRemotePath[BSE_STACK_BUFFER_SMALL];
  string_format(fullRemotePath, sizeof(fullRemotePath), mRemoteBaseDir.utf8().get_data(), "/", mFilename.utf8().get_data());
  mFullRemotePath = fullRemotePath;

  return SSH_OK == _init(user.utf8().get_data(), sshUsername.utf8().get_data(), sshHostname.utf8().get_data(), sshPassword.utf8().get_data());
}

bool AccessCoordinator::is_current_base_config_valid()
{
  if (mSshUsername.is_empty()
  || mSshHostname.is_empty()
  || mSshPassword.is_empty()
  || mRemoteBaseDir.is_empty())
  {
    return false;
  }

  return true;
}

void AccessCoordinator::on_download_dialog_confirm()
{
  download_file(mFullLocalPath.utf8().get_data(), mFullRemotePath.utf8().get_data());
}

void AccessCoordinator::on_upload_dialog_confirm()
{
  if (SSH_OK != reserve_remote_file_for_local_user(mRemoteBaseDir.utf8().get_data(), mFilename.utf8().get_data(), mUser.utf8().get_data(), mIpAddress))
  {
    return; 
  }

  if (SSH_OK != upload_file(mFullLocalPath.utf8().get_data(), mFullRemotePath.utf8().get_data()))
  {
    return;
  }

  release_remote_file_from_local_user(mRemoteBaseDir.utf8().get_data(), mFilename.utf8().get_data(), mUser.utf8().get_data(), mIpAddress, false);
}

void AccessCoordinator::on_force_release_dialog_confirm()
{
  release_remote_file_from_local_user(mRemoteBaseDir.utf8().get_data(), mFilename.utf8().get_data(), mUser.utf8().get_data(), mIpAddress, true);
}

void AccessCoordinator::show_confirmation_dialog(String title, String message, void (AccessCoordinator::*on_confirm)())
{
#if defined(FAC_WINGUI)
  if (mConfirmationDialogCallback && !mAgreeAllPrompts)
  {
    if(mConfirmationDialogCallback(title.get_data(), message.get_data()))
    {
      (this->*on_confirm)();
    }
  }
  else
  {
      (this->*on_confirm)();
  }
#elif defined(NO_GODOT) && !defined(FAC_WINGUI)
  std::string input;
  if (!mAgreeAllPrompts)
  {
    std::cout << message.str;
    std::cout << " (y/n): ";
    std::getline(std::cin, input);
  }

  if (mAgreeAllPrompts || input == "y" || input == "Y")
  {
    (this->*on_confirm)();
  }
#else
  if (mConfirmationDialog)
  {
    mConfirmationDialog->queue_free();
    mConfirmationDialog = nullptr;
  }

  mConfirmationDialog = memnew(ConfirmationDialog);
  mConfirmationDialog->set_text(message);
  mConfirmationDialog->set_title(title);
  mConfirmationDialog->get_ok_button()->set_text("Yes");
  mConfirmationDialog->get_cancel_button()->set_text("No");
  mConfirmationDialog->connect("confirmed", callable_mp(this, on_confirm));

  get_tree()->get_root()->add_child(mConfirmationDialog);

  mConfirmationDialog->popup_centered();
  mConfirmationDialog->show();
#endif
}

constexpr INLINE float as_megabytes( s64 bytes ) { return float(bytes)/(1024.0f * 1024.0f); }

int AccessCoordinator::request_exec(ssh_session session, char const* request, char* buffer, int bufferSize, bool outputToStandardOut)
{
  ssh_channel channel = ssh_channel_new(session);

  if (channel == NULL)
  {
    fprintf(stderr, "Failed to create channel.");
    return SSH_ERROR;
  }

  int rc = ssh_channel_open_session(channel);
  if (rc != SSH_OK)
  {
    ssh_channel_free(channel);
    return SSH_ERROR;
  }

  rc = ssh_channel_request_exec(channel, request); 
  if (rc != SSH_OK)
  {
    log_error("Error on request: ", request); 
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    return SSH_ERROR;
  }

  int bytesRead;

  char* ptr = buffer;
  int sizeLeft = bufferSize - 1;

  if(sizeLeft > 0) ptr[0] = '\0';

  while ((bytesRead = ssh_channel_read(channel, ptr, sizeLeft, 1)) > 0) 
  {
    if (outputToStandardOut) fwrite(ptr, 1, bytesRead, stderr);
    ptr += bytesRead;
    sizeLeft -= bytesRead;
  }

  while ((bytesRead = ssh_channel_read(channel, ptr, sizeLeft, 0)) > 0) 
  {
    if (outputToStandardOut) fwrite(ptr, 1, bytesRead, stdout);
    ptr += bytesRead;
    sizeLeft -= bytesRead;
  }

  if ( sizeLeft < bufferSize ) ptr[0] = '\0';

  ssh_channel_send_eof(channel);
  ssh_channel_close(channel);
  ssh_channel_free(channel);

  return SSH_OK;
}

int AccessCoordinator::request_exec(char const* request)
{
  char buffer[256];
  buffer[0] = '\0';
  return request_exec(mSession, request, buffer, sizeof(buffer));
}

int AccessCoordinator::log_section_exit_return_error()
{
  log_info("==============================================\n");
  return SSH_ERROR;
}

int AccessCoordinator::upload_file(const char* localPath, char const* remotePath)
{
  log_info("\n==============================================");
  log_info("--- Uploading File ---------------------------");

  log_info("- Attempting to upload '", localPath, "' as '", remotePath, "'");

  sftp_session sftp = sftp_new(mSession);

  if (sftp == NULL)
  {
    log_error("- Error creating SFTP session: ", ssh_get_error(mSession));
    return log_section_exit_return_error();
  }

  if (sftp_init(sftp) != SSH_OK)
  {
    log_error("- Error initializing SCP: ", ssh_get_error(mSession));
    sftp_free(sftp);
    return log_section_exit_return_error();
  }

  FILE* localFile = fopen(localPath, "rb");
  if (!localFile)
  {
    log_error("- Error opening local file '", localPath, "', are you sure it exists?");
    sftp_free(sftp);
    return log_section_exit_return_error();
  }
  char stringFormatBuffer[BSE_STACK_BUFFER_SMALL];
  char responseBuffer[BSE_STACK_BUFFER_SMALL];
  responseBuffer[0] = '\0';
  //Upload to _PART instead of the file directly and overwrite the actual file afterwards
  string_format(stringFormatBuffer, sizeof(stringFormatBuffer), remotePath, "_PART");

  sftp_file remoteFile = sftp_open(sftp, stringFormatBuffer, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (remoteFile == NULL)
  {
    log_error("- Error opening remote file: ", ssh_get_error(mSession));
    sftp_free(sftp);
    return log_section_exit_return_error();
  }

  fseek(localFile, 0, SEEK_END);
  s64 localFileSize = ftell(localFile);
  fseek(localFile, 0, SEEK_SET);


  char buffer[BSE_STACK_BUFFER_GARGANTUAN_PLUS];
  int bytesRead;

  using clock = std::chrono::steady_clock;
  auto nextLogClock = clock::now() + std::chrono::seconds(1);
#if defined(FAC_WINGUI) && defined(POLL_WINDOWS_IN_SFTP_THREAD)
  auto nextWindowPollClock = clock::now() + std::chrono::milliseconds(100);
#endif

  int bytesWrittenTotal = 0;
  int bytesSinceLastSecond = 0;
  while ((bytesRead = (int)fread(buffer,1, sizeof(buffer), localFile)) > 0)
  {
    if (sftp_write(remoteFile, buffer, bytesRead) != bytesRead)
    {
      log_error("- Error while writing remote file: ", ssh_get_error(mSession));
      break;
    }

    bytesWrittenTotal += bytesRead;
    bytesSinceLastSecond += bytesRead;

    auto now = clock::now();
    if (now >= nextLogClock)
    {
      //string_format(stringFormatBuffer, sizeof(stringFormatBuffer), "echo \"$(stat -c%s '", mFullRemotePath.utf8().get_data(), "')\"");
      //request_exec(mSession, stringFormatBuffer, responseBuffer, sizeof(responseBuffer), false);
      //s64 remoteFileBytes = 0;
      //string_parse_value(responseBuffer, &remoteFileBytes);
      s64 remoteFileBytes = bytesWrittenTotal;
#if defined(FAC_WINGUI)
      log_info("\rProgress: ", remoteFileBytes, "/", localFileSize, " Bytes                Speed: ", as_megabytes(bytesSinceLastSecond), " MB/s");
#else
      std::cout << "\rProgress: " << remoteFileBytes << "/" << localFileSize << " Bytes                Speed: " << as_megabytes(bytesSinceLastSecond) << " MB/s" << std::flush;   
#endif
      bytesSinceLastSecond = 0;  
      #if defined(FAC_WINGUI) && defined(POLL_WINDOWS_IN_SFTP_THREAD)
      mWindowMessageCallback(true);
      #endif
      nextLogClock = now + std::chrono::seconds(1);
    }
  }
#if defined(FAC_WINGUI)
  log_info("\rProgress: Done! ");
#else
  std::cout << "\r\n";
#endif

  string_format(stringFormatBuffer, sizeof(stringFormatBuffer), "echo \"$(stat -c%s '", remotePath, "_PART')\" "
        "&& if [ \"$(stat -c%s '", remotePath, "_PART')\" -eq ", localFileSize, " ]; then mv -f '",remotePath,"_PART' '", remotePath, "'; fi");
  request_exec(mSession, stringFormatBuffer, responseBuffer, sizeof(responseBuffer), false);
  
  s64 remoteFileSize;
  string_parse_value(responseBuffer, &remoteFileSize);

  int result;
  if (s64(remoteFileSize) == localFileSize)
  {
#if defined(_WIN32)
  if (SetFileAttributes(localPath, FILE_ATTRIBUTE_READONLY) == 0)
  {
    log_error("- Error setting file readonly.");
  }
#endif

    log_info("                         \r\n- Success!\r\n");
    log_info("- Uploaded: ", as_megabytes(localFileSize), " MB");
    result = SSH_OK;
  }
  else
  {
    log_error("- Error while uploading, local file is ", localFileSize, " bytes large while the remote file is ", remoteFileSize, " bytes large.");
    result = SSH_ERROR;
  }

  log_info("==============================================\n");

  sftp_close(remoteFile);
  fclose(localFile);
  sftp_free(sftp);
  return result;
}

int AccessCoordinator::download_file(char const* localPath, char const* remotePath)
{
  log_info("\n==============================================");
  log_info("--- Downloading File -------------------------");
  
  log_info("- Attempting to download '", remotePath, "' as '", localPath, "'");

  sftp_session sftp = sftp_new(mSession);
  if (sftp == NULL)
  {
    log_error("- Error creating SFTP session: ", ssh_get_error(mSession));
    return log_section_exit_return_error();
  }

  if (sftp_init(sftp) != SSH_OK)
  {
    log_error("- Error initializing SCP: ", ssh_get_error(mSession));
    sftp_free(sftp);
    return log_section_exit_return_error();
  }
  
  char stringFormatBuffer[BSE_STACK_BUFFER_SMALL];
  char responseBuffer[BSE_STACK_BUFFER_SMALL];
  responseBuffer[0] = '\0';

//  string_format(stringFormatBuffer, sizeof(stringFormatBuffer), "if [ \"$(stat -c%s '", remotePath, "')\" -lt \"$(stat -c%s '", remotePath, "_BACKUP')\" ]; "
//                                  "then cp -f '", remotePath, "_BACKUP' '", remotePath, "'; fi");
//  request_exec(mSession, stringFormatBuffer, responseBuffer, sizeof(responseBuffer), false);

  sftp_file remoteFile = sftp_open(sftp, remotePath, O_RDONLY, 0);
  if (remoteFile == NULL)
  {
    log_error("- Error opening remote file: ", ssh_get_error(mSession));
    sftp_free(sftp);
    return log_section_exit_return_error();
  }

  #if defined(_WIN32)
  //Don't care if file doesn't exist
  SetFileAttributes(localPath, FILE_ATTRIBUTE_NORMAL);
  #endif

  string_format(stringFormatBuffer, sizeof(stringFormatBuffer), "echo \"$(stat -c%s '", remotePath, "')\"");
  request_exec(mSession, stringFormatBuffer, responseBuffer, sizeof(responseBuffer), false);
  
  s64 remoteFileSize;
  string_parse_value(responseBuffer, &remoteFileSize);

  if (remoteFileSize == 0)
  {
    log_info("- Remote file has size 0, local file won't be overwritten.");
    return log_section_exit_return_error();
  }

  FILE* localFile = fopen(localPath, "wb");
  if (!localFile)
  {
    log_error("- Error opening local file '", localPath, "', are you sure it exists?");
    sftp_close(remoteFile);
    sftp_free(sftp);
    return log_section_exit_return_error();
  }

  char buffer[BSE_STACK_BUFFER_GARGANTUAN_PLUS];
  int bytesRead;

  using clock = std::chrono::steady_clock;
  auto nextLogClock = clock::now() + std::chrono::seconds(1);
#if defined(FAC_WINGUI) && defined(POLL_WINDOWS_IN_SFTP_THREAD)
  auto nextWindowPollClock = clock::now() + std::chrono::milliseconds(100);
#endif
  s64 bytesSinceLastSecond = 0;
  s64 totalBytesRead = 0;
  while ((bytesRead = (int)sftp_read(remoteFile, buffer, sizeof(buffer))) > 0)
  {
    if (fwrite(buffer, 1, bytesRead, localFile) != (size_t)bytesRead)
    {
      log_error("- Error writing local file: ", localPath);
      break;
    }

    auto now = clock::now();
    bytesSinceLastSecond += bytesRead;
    totalBytesRead += bytesRead;
    if (now >= nextLogClock)
    {
#if defined(FAC_WINGUI)
      log_info("\rProgress: ", totalBytesRead, "/", remoteFileSize, " Bytes                Speed: ", as_megabytes(bytesSinceLastSecond), " MB/s");
#else
  std::cout << "\rProgress: " << totalBytesRead << "/" << remoteFileSize << " Bytes                Speed: " << as_megabytes(bytesSinceLastSecond) << " MB/s" << std::flush;   
#endif
      #if defined(FAC_WINGUI) && defined(POLL_WINDOWS_IN_SFTP_THREAD)
      mWindowMessageCallback(true);
      #endif
      nextLogClock = now + std::chrono::seconds(1);
      bytesSinceLastSecond = 0;
    }
  }
#if defined(FAC_WINGUI)
  log_info("\rProgress: Done! ");
#else
  std::cout << "\r\n";
#endif

  if (bytesRead < 0)
  {
    log_error("- Error while reading remote file: ", ssh_get_error(mSession));
  }

  fseek(localFile, 0, SEEK_END);
  s64 localFileSize = ftell(localFile);

  if (s64(remoteFileSize) == localFileSize)
  {
    log_info("                         \r\n- Success!\r\n");
    log_info("- Downloaded: ", as_megabytes(localFileSize), " MB");
  }
  else
  {
    log_error("- Error while downloading, local file is ", localFileSize, " bytes large while the remote file is ", remoteFileSize, " bytes large.");
    bytesRead = -1;
  }

  log_info("==============================================\n");

  fclose(localFile);
  sftp_close(remoteFile);
  sftp_free(sftp);
  return bytesRead < 0 ? SSH_ERROR : SSH_OK;
}

int AccessCoordinator::_init(char const* user, char const* sshUser, char const* sshHost, char const* sshPassword)
{
  log_info("\n==============================================");
  log_info("--- Launching Access Session -----------------");

  mSession = ssh_new();

  if (mSession == nullptr)
  {
    log_error("- Failed to create ssh session.");
    return log_section_exit_return_error();
  }

  ssh_options_set(mSession, SSH_OPTIONS_HOST, sshHost);
  ssh_options_set(mSession, SSH_OPTIONS_USER, sshUser);

  int rc = ssh_connect(mSession);
  if (rc != SSH_OK)
  {
    log_error("- Connection failed: ", ssh_get_error(mSession));
    ssh_free(mSession);
    mSession = nullptr;
    return log_section_exit_return_error();
  }

  rc = ssh_userauth_password(mSession, NULL, sshPassword);
  if (rc != SSH_AUTH_SUCCESS)
  {
    log_error("- Authentication failed: ", ssh_get_error(mSession));
    ssh_free(mSession);
    mSession = nullptr;
    return log_section_exit_return_error();
  }
  
  char stringFormatBuffer[BSE_STACK_BUFFER_LARGE];
  char responseBuffer[256];
  responseBuffer[0] = '\0';
  
  string_format(stringFormatBuffer, sizeof(stringFormatBuffer), "echo $SSH_CONNECTION | awk '{print $1}'");
  request_exec(mSession, stringFormatBuffer, responseBuffer, sizeof(responseBuffer), false);
  string_copy(mIpAddress, responseBuffer, sizeof(mIpAddress));
  string_replace_char(mIpAddress,'\n','\0');

  log_info("- Connected to '", sshUser, "@", sshHost, "'");
  log_info("- User: ", user);
  log_info("- Local IP: ", mIpAddress);

  log_info("==============================================\n");

  return SSH_OK;
}

int AccessCoordinator::shutdown_session()
{
  ssh_disconnect(mSession);
  ssh_free(mSession);
  return SSH_OK;
}

int AccessCoordinator::reserve_remote_file_for_local_user( char const* remoteBaseDir, char const* filename, char const* user, char const* myIp)
{
  if (mReservedFileCache.find(filename) != -1)
  {
    return SSH_OK;
  }

  log_info("\n==============================================");
  log_info("--- Reserving File ---------------------------");
  log_info("- Attempting to reserve '", remoteBaseDir, "/", filename, "' for user '", user, "' with ip '", myIp, "'");

  char responseBuffer[BSE_STACK_BUFFER_SMALL];
  responseBuffer[0] = '\0';
  char stringFormatBuffer[BSE_STACK_BUFFER_LARGE];
  stringFormatBuffer[0] = '\0';
  // With 'examplefile' this will create an '_examplefilereservation' and enter the username and ip to reserve.
  // If 'examplefile' has write permission and '_examplefilereservation' is empty, we can reserve it.
  string_format(stringFormatBuffer, sizeof(stringFormatBuffer), 
               "cd '", remoteBaseDir, "' && touch '", filename, "' && touch '_", filename, "reservation'"
               " && if [ \"$(stat -c%a '", filename, "')\" -eq 644 ] && [ \"$(stat -c%s '_", filename, "reservation')\" -eq 0 ]; "
                "then echo \"", user, " ", myIp ,"\" > '_", filename,"reservation'; fi");
  request_exec(mSession, stringFormatBuffer, responseBuffer, sizeof(responseBuffer));
  string_format(stringFormatBuffer, sizeof(stringFormatBuffer), 
               "cd '", remoteBaseDir,
               "' && if [ \"$(stat -c%a '", filename, "')\" -eq 644 ] && grep -q \"", user, " ", myIp , "\" '_", filename, "reservation'; "
               "then chmod 444 '", filename, "' && echo \"- Successfully reserved '", filename, "'!\"; elif grep -q \"", user, " ", myIp , "\" '_", filename, "reservation'; then echo \"- File '", filename, "' is already reserved by you.\"; else echo \"- Can't reserve file '", filename, "', it's already reserved by '$(cat '_", filename, "reservation')'\"; fi"); 
  responseBuffer[0] = '\0';
  request_exec(mSession, stringFormatBuffer, responseBuffer, sizeof(responseBuffer), false);
  log_info(responseBuffer);

  log_info("==============================================\n");

  int result = string_begins_with(responseBuffer, "- C") ? SSH_ERROR : SSH_OK;
  if (result == SSH_OK)
  {
    mReservedFileCache.push_back(filename);
  }

  return result;
}


int AccessCoordinator::release_remote_file_from_local_user( char const* remoteBaseDir, char const* filename, char const* user, char const* myIp, bool overridePermissions)
{
  int cached = mReservedFileCache.find(filename);
  if (cached >= 0) mReservedFileCache.remove_at(cached);

  log_info("\n==============================================");
  log_info("--- Releasing File ---------------------------");

  char stringFormatBuffer[BSE_STACK_BUFFER_LARGE];
  stringFormatBuffer[0] = '\0';
  char responseBuffer[BSE_STACK_BUFFER_SMALL];
  responseBuffer[0] = '\0';

  if (overridePermissions)
  {
    string_format(stringFormatBuffer, sizeof(stringFormatBuffer), 
               "cd '", remoteBaseDir, "' && touch '", filename, "' && touch '_", filename, "reservation'"
               " && chmod 644 '", filename, "' && echo \"- Force releasing '", filename, "' from '$(cat \"_", filename, "reservation\")'\" && truncate -s 0 '_", filename, "reservation'"); 
  }
  else
  {
    log_info("- Attempting to release '", remoteBaseDir, "/", filename, "' from user '", user, "' with ip '", myIp, "'");
    string_format(stringFormatBuffer, sizeof(stringFormatBuffer), 
               "cd '", remoteBaseDir,
               "' && if grep -q \"", user, " ", myIp , "\" '_", filename, "reservation'; "
               "then chmod 644 '", filename, "' && truncate -s 0 '_", filename, "reservation' && echo \"- Successfully released '", filename, "'!\"; else echo \"- File '", filename, "' is reserved by '$(cat \"_", filename, "reservation\")', you don't have permission to release it\"; fi"); 
  }

  request_exec(mSession, stringFormatBuffer, responseBuffer, sizeof(responseBuffer), false);
  log_info(responseBuffer);

  log_info("==============================================\n");

  return SSH_OK;
}

ReserveState AccessCoordinator::get_reserve_state_of_remote_file(String* outOwner, s64* outFileSize, char const* remoteBaseDir, char const* filename, char const* user, char const* myIp)
{
  if (outOwner) *outOwner = "";
  if (outFileSize) *outFileSize = 0;

  char responseBuffer[BSE_STACK_BUFFER_SMALL];
  responseBuffer[0] = '\0';
  responseBuffer[1] = '\0';
  responseBuffer[2] = '\0';
  char stringFormatBuffer[BSE_STACK_BUFFER_LARGE];
  stringFormatBuffer[0] = '\0';
  //0 = file doesn't exist
  //A = free
  //B = owned by me followed by the size of the file on the server
  //C = owned by other followed by the name and ip of the owner
  string_format(stringFormatBuffer, sizeof(stringFormatBuffer), 
               "cd '", remoteBaseDir,
               "' && if [ ! -f ", filename, " ]; then echo \"0\"; elif [ \"$(stat -c%a '", filename, "')\" -eq 644 ] && [ \"$(stat -c%s '_", filename, "reservation')\" -eq 0 ]; "
               "then echo \"A\"; elif grep -q \"", user, " ", myIp , "\" '_", filename, 
               "reservation'; then echo \"B $(stat -c%s '", filename, "')\"; else echo \"C $(cat '_", filename, "reservation')\"; fi"); 
  request_exec(mSession, stringFormatBuffer, responseBuffer, sizeof(responseBuffer), false);

  ReserveState result;
  if (responseBuffer[0] == '0')
  {
    result = ReserveState::UNKNOWN;
  }
  else if (responseBuffer[0] == 'A')
  {
    result = ReserveState::NOT_RESERVED;
  }
  else if (responseBuffer[0] == 'B')
  {
    if(outFileSize && string_length(responseBuffer) > 2) string_parse_value(responseBuffer + 2, outFileSize);
    result = ReserveState::RESERVED_BY_ME;
  }
  else if (responseBuffer[0] == 'C')
  {
    if (outOwner && string_length(responseBuffer) > 2) *outOwner = (responseBuffer + 2);
    result = ReserveState::RESERVED_BY_OTHER;
  }

  int cached = mReservedFileCache.find(filename);
  if (cached >= 0 && result != ReserveState::RESERVED_BY_ME) mReservedFileCache.remove_at(cached);

  return result;
}
