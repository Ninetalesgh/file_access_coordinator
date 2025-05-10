extends Node2D


var filename: String
var user: String
var sshUsername: String
var sshHostname: String
var sshPassword: String
var remoteBaseDir: String

func _ready() -> void:
  reload_config()
  var nativeSsh := AccessCoordinator.new()
  nativeSsh.init(filename, user, sshUsername, sshHostname, sshPassword, remoteBaseDir)
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
      sshUsername = line.substr(12)
    elif line.begins_with("sshUsername="):
      sshHostname = line.substr(12)
    elif line.begins_with("sshPassword="):
      sshPassword = line.substr(12)
    elif line.begins_with("remoteBaseDir="):
      remoteBaseDir = line.substr(14)
  pass


func _on_download_button_pressed() -> void:
  print("_on_download_button_pressed")

func _on_upload_button_pressed() -> void:
  print("_on_upload_button_pressed")

func _on_release_pressed() -> void:
  print("_on_release_pressed")

func _on_reserve_pressed() -> void:
  print("_on_reserve_pressed")
