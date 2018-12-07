


Sign_Target:
   COPY /Y .\FragFlt.inf $(O)\FragFlt.inf
#   COPY /Y .\FragFlt.cdf $(O)\FragFlt.cdf
   inf2cat /driver:$(O) /os:Vista_X64 /verbose
!IF "$(TEST_SIGN_TARGET)" == "1"
   signtool sign /v /s PrivateCertStore /n "mutexed.com(Test)" /t http://timestamp.verisign.com/scripts/timestamp.dll "$(TARGET)"
   signtool verify /v /pa "$(TARGET)"
   signtool sign /v /s PrivateCertStore /n "mutexed.com(Test)" /t http://timestamp.verisign.com/scripts/timestamp.dll "$(TARGET:.sys=.cat)"
!ENDIF
   
PostBuild_Target: Sign_Target
#   IF EXIST PostBuild.bat PostBuild.bat $(TARGET)
