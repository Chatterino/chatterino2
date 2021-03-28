# Enteprise deployment guide
**This guide is exclusive to and assumes that you're using Microsoft's Windows Active Directory Domain Services infrastructure**

# Requirements:

- Windows Active Directory Domain Services
 - Windows Active Directory Certificate Services
 - User certificate with code signing rights exported WITH private key
 - Microsoft's MSIX packaging tool
 
# Package creation for deployment

1. Open MSIX packaging tool
2. Select task "Application package: create your app package"
3. Select the packaging method of your choice
4. In the "Prepare computer" screen enable the recommend action items and press next
5. In the "Choose the installer you want to package" screen pick the chatterino installer and select your certificate. Press next.
6. In the "Package information" screen, you may set the info required as you want. The "Installation location" may be left empty. Press next.
7. The Chatterino install window is going to pop up. Select the "Install for all users (recommended)".
8. Go through the installer as normal, preferably selecting all options, except the last one "Launch chatterino".
9. Click next on the MSIX screen.
10. You're now in the "Manage first launch tasks" screen. Pick "Browse" and select the file "unins000.exe". Now click on next.
11. On the "Services report" window, you're gonna see an empty list. That's expected. Click next.
12. On the "Save Package" screen select "Package editor".
13. In the "Create Package" screen, select your preffered save location and click on "Create".
14. You now should see the "Package successfully created" window. You can close it.

After creating the package, manually install it once to test that it's working and that you're not getting any certificate errors. If you do, your certificate infrastructure is setup improperly (chain is not being imported into domain joined computers) or the computer is not domain joined.

# Package deployment

This will guide you through a very basic deployment setup without much restrictions in place. It should be seen as more of a template to get it going as seen fit.
This also does not rely on Microsoft's current tactic of killing off Group Policy Program deployment that FORCES you to buy new products and licensing instead of relying on your single Windows Server Licensing. Eg: Intune, SCCM etc.

These instructions are to be followed on the PDC or through RSAT

1. Open the group policy manager
2. Create a new group policy or select an existing one
3. Select either "Computer Configuration" or "User configuration", dependent on how you want to assign the deployment.
4. Go into "Policies->Administrative Templates->Windows Components->App Package Deployment"
5. Double click the "Allow all trusted apps to install" and set it to enabled.
6. Now go into "Policies->Windows Configuration->Scripts->Logon->Powershell Scripts"
7. Add a PowerShell script with the following content, hosted in a network share accessible by the computers or users to which you're applying the policy:

        Add-AppxPackage -Path "\\ServerName\SharePath\Chatterino.msix"
8. Click apply and close the group policy management.

Now, upon login, the users and or computers to which you applied the policy will have Chatterino installed by default.
This guide can be expanded upon with registry key changes within group policy for further lock down of Chatterino settings in an Enterprise environment and different deployments.










