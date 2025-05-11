extends Node2D


var filepath: String
var user: String
var sshUsername: String
var sshHostname: String
var sshPassword: String
var remoteBaseDir: String

var hasChanged = true

var accessCoordinator = AccessCoordinator.new()

@onready var output = %output
@onready var text_user = %text_user
@onready var text_filepath = %text_filepath

func set_filepath(_filepath: String):
  filepath = _filepath
  hasChanged = true
func set_user(_user: String):
  user = _user
  hasChanged = true
func set_sshUsername(_sshUsername: String):
  sshUsername = _sshUsername
  hasChanged = true
func set_sshHostname(_sshHostname: String):
  sshHostname = _sshHostname
  hasChanged = true
func set_sshPassword(_sshPassword: String):
  sshPassword = _sshPassword
  hasChanged = true
func set_remoteBaseDir(_remoteBaseDir: String):
  remoteBaseDir = _remoteBaseDir
  hasChanged = true

func _ready() -> void:
  add_child(accessCoordinator)
  load_config()
  pass

func _log(message: String):
  output.text += message
  output.set_v_scroll(output.get_v_scroll_bar().get_max())

func _process(_delta: float) -> void:
  var fetched = accessCoordinator.fetch_output()
  if !fetched.is_empty():
    _log(fetched)

func load_config():
  var configRes = "res://access.config"
  var file = FileAccess.open(configRes, FileAccess.READ)
  var configText = file.get_as_text()
  var lines = configText.split("\n")
  
  for line in lines:
    if line.begins_with("filepath="):
      filepath = line.substr(9)
    elif line.begins_with("user="):
      user = line.substr(5)
    elif line.begins_with("sshHostname="):
      sshHostname = line.substr(12)
    elif line.begins_with("sshUsername="):
      sshUsername = line.substr(12)
    elif line.begins_with("sshPassword="):
      sshPassword = line.substr(12)
    elif line.begins_with("remoteBaseDir="):
      remoteBaseDir = line.substr(14)
  
  if sshHostname.is_empty() || sshUsername.is_empty() || sshPassword.is_empty() || remoteBaseDir.is_empty():
    _log("- Error: access.config does not have the necessary declarations. Needed are:\nsshHostname=\nsshUsername\nsshPassword\nremoteBaseDir=\n")
  
  text_user.text = user
  text_filepath.text = filepath
  file.close()

func _on_save_config_button_pressed():
  if _check_cache():
    var configRes = "res://access.config"
    var file = FileAccess.open(configRes, FileAccess.WRITE)
    print(file.get_as_text())
    file.store_line("filepath=" + filepath)
    file.store_line("user=" + user)
    file.store_line("sshHostname=" + sshHostname)
    file.store_line("sshUsername=" + sshUsername)
    file.store_line("sshPassword=" + sshPassword)
    file.store_line("remoteBaseDir=" + remoteBaseDir)
    file.close()
    _log("- Saved access.config.\n")
  else:
    _log("- Error: Can't write config file, current loaded config is faulty.\n")

func _on_download_button_pressed() -> void:
  if _check_cache():
    _update_native_config()
    _log_time()
    accessCoordinator.download()

func _on_upload_button_pressed() -> void:
  if _check_cache():
    _update_native_config()
    _log_time()
    accessCoordinator.upload()

func _on_release_button_pressed() -> void:
  if _check_cache():
    _update_native_config()
    _log_time()
    accessCoordinator.release(false)

func _on_force_release_button_pressed() -> void:
  if _check_cache():
    _update_native_config()
    _log_time()
    accessCoordinator.release(true)

func _on_reserve_button_pressed() -> void:
  if _check_cache():
    _update_native_config()
    _log_time()
    accessCoordinator.reserve()


func _check_cache() -> bool:
  if !text_user.text.is_empty() && text_user.text != user:
    user = text_user.text
    hasChanged = true
  if !text_filepath.text.is_empty() && text_filepath.text != filepath:
    filepath = text_filepath.text
    hasChanged = true
  return _check_config()

func _update_native_config():
  if hasChanged:
    accessCoordinator.init(filepath, user, sshUsername, sshHostname, sshPassword, remoteBaseDir)
    hasChanged = false

func _check_config() -> bool:
  if filepath.is_empty() || sshHostname.is_empty() || sshUsername.is_empty() || sshPassword.is_empty() || remoteBaseDir.is_empty():
    return false
  return true

func _log_time():
  var time = Time.get_datetime_dict_from_system()
  var formattedTime = "\n" + str(time["year"]) + "-" + str(time["month"]).pad_zeros(2) + "-" + str(time["day"]).pad_zeros(2) + " " + str(time["hour"]).pad_zeros(2) + ":" + str(time["minute"]).pad_zeros(2) + ":" + str(time["second"]).pad_zeros(2)
  _log(formattedTime)