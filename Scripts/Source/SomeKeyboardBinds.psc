scriptName SomeKeyboardBinds extends Quest

function EvalJs(string jsCode) global native

event OnInit()
    RegisterForKey(0xD3) ; Delete
    RegisterForKey(0xD2) ; Insert
endEvent

event OnKeyDown(int keyCode)
    if keyCode == 0xD3 ; Delete
        PromptForJavaScriptToRun()
    elseIf keyCode == 0xD2 ; Insert
        RunJavaScriptFile()
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

function RunJavaScriptFile()
    if MiscUtil.FileExists("Data/javascript.js")
        string javaScriptCode = MiscUtil.ReadFromFile("Data/javascript.js")
        if javaScriptCode
            EvalJs(javaScriptCode)
        endIf
    endIf
endFunction
