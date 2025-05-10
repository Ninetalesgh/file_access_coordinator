extends Node2D


var filename: String
var user: String
var sshUsername: String
var sshHostname: String
var sshPassword: String
var remoteBaseDir: String

var hasChanged = true

var accessCoordinator = AccessCoordinator.new()

func set_filename(_filename: String):
  filename = _filename
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
  reload_config()
  pass

func _process(delta: float) -> void:
  pass

func reload_config():
  var configRes = "res://access.config"
  var file = FileAccess.open(configRes, FileAccess.READ)
  var configText = file.get_as_text()
  var lines = configText.split("\n")
  
  for line in lines:
    if line.begins_with("filename="):
      filename = line.substr(9)
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
  pass

func _on_download_button_pressed() -> void:
  _check_cache()
  accessCoordinator.download()

func _on_upload_button_pressed() -> void:
  _check_cache()
  accessCoordinator.upload()

func _on_release_pressed() -> void:
  _check_cache()
  accessCoordinator.release(false)

func _on_reserve_pressed() -> void:
  _check_cache()
  accessCoordinator.reserve()

func _check_cache():
  if hasChanged:
    accessCoordinator.init(filename, user, sshUsername, sshHostname, sshPassword, remoteBaseDir)
    hasChanged = false
