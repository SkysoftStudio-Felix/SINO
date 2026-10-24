Sub Form_Load()
  Dim my_value As Long
  my_value = 0
  Result_Label.Caption = "Ready"
End Sub

Sub Run_Button_Click()
  my_value = my_value + 1
  Result_Label.Caption = "Value: " & CStr(my_value)
  Print Result_Label.Caption
End Sub
