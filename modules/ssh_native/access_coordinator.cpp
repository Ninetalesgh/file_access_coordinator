#include "debug.h"

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
  //This entire output forwarding thing is messy, but for the sake of this program and the time I want to put into it.. what can you do.
  String result = output;
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

bool AccessCoordinator::init(String filepath, String user, String sshUsername, String sshHostname, String sshPassword, String remoteBaseDir)
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
#ifdef NO_GODOT
  std::string input;
  std::cout << message.str;
  std::cout << "Are you sure? (y/n): ";
  std::getline(std::cin, input);

  if (input == "y" || input == "Y")
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

int AccessCoordinator::request_exec(char const* request, char* buffer, int bufferSize, bool outputToStandardOut)
{
  ssh_channel channel = ssh_channel_new(mSession);

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
  return request_exec(request, buffer, sizeof(buffer));
}

int AccessCoordinator::log_section_exit_return_error()
{
  log_info("----------------------------------------------");
  log_info("==============================================\n");
  return SSH_ERROR;
}

int AccessCoordinator::upload_file(const char* localPath, char const* remotePath)
{
  log_info("\n==============================================");
  log_info("----------------------------------------------");
  log_info("--- Uploading File ---------------------------");
  log_info("----------------------------------------------");

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
    log_error("- Error opening local file: ", localPath);
    sftp_free(sftp);
    return log_section_exit_return_error();
  }

  sftp_file remoteFile = sftp_open(sftp, remotePath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (remoteFile == NULL)
  {
    log_error("- Error opening remote file: ", ssh_get_error(mSession));
    sftp_free(sftp);
    return log_section_exit_return_error();
  }

  char buffer[4096];
  int bytesRead;
  while ((bytesRead = (int)fread(buffer,1, sizeof(buffer), localFile)) > 0)
  {
    if (sftp_write(remoteFile, buffer, bytesRead) != bytesRead)
    {
      log_error("- Error while writing remote file: ", ssh_get_error(mSession));
      break;
    }
  }

  char stringFormatBuffer[BSE_STACK_BUFFER_LARGE];
  char responseBuffer[BSE_STACK_BUFFER_SMALL];
  responseBuffer[0] = '\0';

  string_format(stringFormatBuffer, sizeof(stringFormatBuffer), "echo \"$(stat -c%s '", remotePath, "')\"");
  request_exec(stringFormatBuffer, responseBuffer, sizeof(responseBuffer), false);
  
  int remoteFileSize;
  string_parse_value(responseBuffer, &remoteFileSize);

  fseek(localFile, 0, SEEK_END);
  s64 localFileSize = ftell(localFile);

  int result;
  if (s64(remoteFileSize) == localFileSize)
  {
    log_info("- Success!");
    log_info("- Uploaded size: ", as_megabytes(localFileSize), " MB");
    result = SSH_OK;
  }
  else
  {
    log_error("- Error while uploading, local file is ", localFileSize, " bytes large while the remote file is ", remoteFileSize, " bytes large.");
    result = SSH_ERROR;
  }

  log_info("----------------------------------------------");
  log_info("==============================================\n");

  sftp_close(remoteFile);
  fclose(localFile);
  sftp_free(sftp);
  return result;
}

int AccessCoordinator::download_file(char const* localPath, char const* remotePath)
{
  log_info("\n==============================================");
  log_info("----------------------------------------------");
  log_info("--- Downloading File -------------------------");
  log_info("----------------------------------------------");
  
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
  
  sftp_file remoteFile = sftp_open(sftp, remotePath, O_RDONLY, 0);
  if (remoteFile == NULL)
  {
    log_error("- Error opening remote file: ", ssh_get_error(mSession));
    sftp_free(sftp);
    return log_section_exit_return_error();
  }

  FILE* localFile = fopen(localPath, "wb");
  if (!localFile)
  {
    log_error("- Error opening local file: ", localPath);
    sftp_close(remoteFile);
    sftp_free(sftp);
    return log_section_exit_return_error();
  }

  char buffer[4096];
  int bytesRead;
  while ((bytesRead = (int)sftp_read(remoteFile, buffer, sizeof(buffer))) > 0)
  {
    if (fwrite(buffer, 1, bytesRead, localFile) != (size_t)bytesRead)
    {
      log_error("- Error writing local file: ", localPath);
      break;
    }
  }

  if (bytesRead < 0)
  {
    log_error("- Error while reading remote file: ", ssh_get_error(mSession));
  }

  char stringFormatBuffer[BSE_STACK_BUFFER_LARGE];
  char responseBuffer[BSE_STACK_BUFFER_SMALL];
  responseBuffer[0] = '\0';

  string_format(stringFormatBuffer, sizeof(stringFormatBuffer), "echo \"$(stat -c%s '", remotePath, "')\"");
  request_exec(stringFormatBuffer, responseBuffer, sizeof(responseBuffer), false);
  
  int remoteFileSize;
  string_parse_value(responseBuffer, &remoteFileSize);

  fseek(localFile, 0, SEEK_END);
  s64 localFileSize = ftell(localFile);

  if (s64(remoteFileSize) == localFileSize)
  {
    log_info("- Success!");
    log_info("- Downloaded size: ", as_megabytes(localFileSize), " MB");
  }
  else
  {
    log_error("- Error while downloading, local file is ", localFileSize, " bytes large while the remote file is ", remoteFileSize, " bytes large.");
    bytesRead = -1;
  }

  log_info("----------------------------------------------");
  log_info("==============================================\n");

  fclose(localFile);
  sftp_close(remoteFile);
  sftp_free(sftp);
  return bytesRead < 0 ? SSH_ERROR : SSH_OK;
}

int AccessCoordinator::_init(char const* user, char const* sshUser, char const* sshHost, char const* sshPassword)
{
  log_info("\n==============================================");
  log_info("----------------------------------------------");
  log_info("--- Launching Access Session -----------------");
  log_info("----------------------------------------------");

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
  request_exec(stringFormatBuffer, responseBuffer, sizeof(responseBuffer), false);
  string_copy(mIpAddress, responseBuffer, sizeof(mIpAddress));
  string_replace_char(mIpAddress,'\n','\0');

  log_info("- Connected to '", sshUser, "@", sshHost, "'");
  log_info("- User: ", user);
  log_info("- Local IP: ", mIpAddress);

  log_info("----------------------------------------------");
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
  log_info("----------------------------------------------");
  log_info("--- Reserving File ---------------------------");
  log_info("----------------------------------------------");
  log_info("- Attempting to reserve '", filename, "' for user '", user, "' with ip '", myIp, "'");

  char responseBuffer[BSE_STACK_BUFFER_SMALL];
  responseBuffer[0] = '\0';
  char stringFormatBuffer[BSE_STACK_BUFFER_LARGE];
  stringFormatBuffer[0] = '\0';
  // With 'examplefile' this will create an 'examplefilereservation' and enter the username and ip to reserve.
  // If 'examplefile' has write permission and 'examplefilereservation' is empty, we can reserve it.
  string_format(stringFormatBuffer, sizeof(stringFormatBuffer), 
               "cd '", remoteBaseDir, "' && touch '", filename, "' && touch '_", filename, "reservation'"
               " && if [ \"$(stat -c%a '", filename, "')\" -eq 644 ] && [ \"$(stat -c%s '_", filename, "reservation')\" -eq 0 ]; "
                "then echo \"", user, " ", myIp ,"\" > '_", filename,"reservation'; fi");
  request_exec(stringFormatBuffer, responseBuffer, sizeof(responseBuffer));
  string_format(stringFormatBuffer, sizeof(stringFormatBuffer), 
               "cd '", remoteBaseDir,
               "' && if [ \"$(stat -c%a '", filename, "')\" -eq 644 ] && grep -q \"", user, " ", myIp , "\" '_", filename, "reservation'; "
               "then chmod -w '", filename, "' && echo \"- Successfully reserved '", filename, "'!\"; elif grep -q \"", user, " ", myIp , "\" '_", filename, "reservation'; then echo \"- File '", filename, "' is already reserved by you.\"; else echo \"- Can't reserve file '", filename, "', it's already reserved by '$(cat '_", filename, "reservation')'\"; fi"); 
  responseBuffer[0] = '\0';
  request_exec(stringFormatBuffer, responseBuffer, sizeof(responseBuffer), false);
  log_info(responseBuffer);

  log_info("----------------------------------------------");
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
  log_info("----------------------------------------------");
  log_info("--- Releasing File ---------------------------");
  log_info("----------------------------------------------");

  char stringFormatBuffer[BSE_STACK_BUFFER_LARGE];
  stringFormatBuffer[0] = '\0';
  char responseBuffer[BSE_STACK_BUFFER_SMALL];
  responseBuffer[0] = '\0';

  if (overridePermissions)
  {
    string_format(stringFormatBuffer, sizeof(stringFormatBuffer), 
               "cd '", remoteBaseDir, "' && touch '", filename, "' && touch '_", filename, "reservation'"
               " && chmod +w '", filename, "' && truncate -s 0 '_", filename, "reservation' && echo \"- Force releasing '", filename, "' from '$(cat \"_", filename, "reservation\")'\""); 
  }
  else
  {
    log_info("- Attempting to release '", filename, "' from user '", user, "' with ip '", myIp, "'");
    string_format(stringFormatBuffer, sizeof(stringFormatBuffer), 
               "cd '", remoteBaseDir,
               "' && if grep -q \"", user, " ", myIp , "\" '_", filename, "reservation'; "
               "then chmod +w '", filename, "' && truncate -s 0 '_", filename, "reservation' && echo \"- Successfully released '", filename, "'!\"; else echo \"- File '", filename, "' is reserved by '$(cat \"_", filename, "reservation\")', you don't have permission to release it\"; fi"); 
  }

  request_exec(stringFormatBuffer, responseBuffer, sizeof(responseBuffer), false);
  log_info(responseBuffer);

  log_info("----------------------------------------------");
  log_info("==============================================\n");

  return SSH_OK;
}
