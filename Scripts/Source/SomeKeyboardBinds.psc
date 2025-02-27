scriptName SomeKeyboardBinds extends Quest

function EvalJs(string jsCode) global native

event OnInit()
    RegisterForKey(0xD3) ; Delete
endEvent

event OnKeyDown(int keyCode)
    if keyCode == 0xD3 ; Delete
        PromptForJavaScriptToRun()
    endIf
endEvent

function PromptForJavaScriptToRun()
    UITextEntryMenu textEntry = UIExtensions.GetMenu("UITextEntryMenu") as UITextEntryMenu
    textEntry.OpenMenu()
    string javaScriptCode = textEntry.GetResultString()
    if javaScriptCode
        EvalJs(javaScriptCode)
    endIf
endFunction
