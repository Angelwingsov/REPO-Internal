param([Parameter(Mandatory=$true)][string]$ManagedDirectory)
$ErrorActionPreference = 'Stop'
$game = [Reflection.Assembly]::LoadFrom((Join-Path $ManagedDirectory 'Assembly-CSharp.dll'))
$unity = [Reflection.Assembly]::LoadFrom((Join-Path $ManagedDirectory 'UnityEngine.CoreModule.dll'))
$post = [Reflection.Assembly]::LoadFrom((Join-Path $ManagedDirectory 'Unity.Postprocessing.Runtime.dll'))
$flags = [Reflection.BindingFlags]'Public,NonPublic,Instance,Static'
$checks = 0
function Check([bool]$condition, [string]$description) {
    if (!$condition) { throw "Visuals contract failed: $description" }
    $script:checks++
}
$fields = @{
    EnvironmentDirector = @()
    PostProcessing = @('volume','grain','bloom','vignette','motionBlur','lensDistortion','chromaticAberration','colorGrading')
    CameraShake = @('Strength','RotXOld','RotXNew','RotYOld','RotYNew','RotZOld','RotZNew','PosXOld','PosXNew','PosYOld','PosYNew','PosZOld','PosZNew')
    CameraNoise = @('StrengthDefault','Strength','OverrideStrength')
    CameraGlitch = @('ActiveParent','GlitchLongCount','GlitchShortCount','GlitchTinyCount','doNotLookEffectTimer','doNotLookEffectImpulseTimer')
    HurtVignette = @('image')
}
foreach ($name in $fields.Keys) {
    $type = $game.GetType($name)
    Check ($null -ne $type) $name
    $update = $type.GetMethod('Update', $flags)
    Check ($null -ne $update -and !$update.IsStatic -and $update.GetParameters().Count -eq 0 -and $update.ReturnType -eq [void]) "$name.Update signature"
    foreach ($field in $fields[$name]) { Check ($null -ne $type.GetField($field,$flags)) "$name.$field" }
}
$render = $unity.GetType('UnityEngine.RenderSettings')
foreach ($name in @('fog','ambientLight','ambientIntensity','ambientMode')) {
    $getter = $render.GetMethod("get_$name", $flags)
    $setter = $render.GetMethod("set_$name", $flags)
    Check ($null -ne $getter -and $getter.IsStatic -and $getter.GetParameters().Count -eq 0) "RenderSettings.get_$name"
    Check ($null -ne $setter -and $setter.IsStatic -and $setter.GetParameters().Count -eq 1) "RenderSettings.set_$name"
}
$mode = $unity.GetType('UnityEngine.Rendering.AmbientMode')
Check ([int][Enum]::Parse($mode,'Flat') -eq 3) 'AmbientMode.Flat enum value'
$settings = $post.GetType('UnityEngine.Rendering.PostProcessing.PostProcessEffectSettings')
$activeField = $settings.GetField('active',$flags)
Check ($null -ne $activeField -and $activeField.FieldType -eq [bool]) 'PostProcessEffectSettings.active is bool'
foreach ($name in @('Grain','Bloom','Vignette','MotionBlur','LensDistortion','ChromaticAberration','ColorGrading')) {
    Check ($settings.IsAssignableFrom($post.GetType("UnityEngine.Rendering.PostProcessing.$name"))) "$name inherits effect settings"
}
$volume = $post.GetType('UnityEngine.Rendering.PostProcessing.PostProcessVolume')
Check ($volume.GetField('weight',$flags).FieldType -eq [single]) 'PostProcessVolume.weight is float'
foreach ($spec in @(@('Behaviour','get_enabled',0),@('Behaviour','set_enabled',1),@('GameObject','get_activeSelf',0),@('GameObject','SetActive',1))) {
    $method = $unity.GetType("UnityEngine.$($spec[0])").GetMethod($spec[1],$flags)
    Check ($null -ne $method -and !$method.IsStatic -and $method.GetParameters().Count -eq $spec[2]) "$($spec[0]).$($spec[1]) signature"
}
Write-Output "PASS: $checks installed-game metadata contracts"
