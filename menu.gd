extends Node2D


func _ready() -> void:
  var woah := AccessCoordinator.new()
  woah.init()
  pass

func _process(delta: float) -> void:
  pass