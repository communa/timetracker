**Communa TimeTracker** is a cross-platform desktop application designed to streamline and automate time management operations for both remote workers and businesses, enhancing the convenience of remote work.

- [Communa Network](https://communa.network/)
- [TimeTracker GitHub Repository](https://github.com/communa/timetracker)

## **Time Recording**

The desktop app assists remote workers in tracking the time spent on projects throughout the day. It operates in the background, automatically recording working hours, with the total hours displayed in the system tray. The app features user-friendly controls, including START/STOP buttons to manage the timer. This ensures accurate recording of working hours, which can later be invoiced to clients. To enhance accuracy, the timer automatically stops after 10 minutes of inactivity, reducing the need for manual pauses.

Tracked time is sent every 10 minutes via an API to our backend, providing instant accessibility on Communa's website. Both freelancers and clients can view this data from any device, facilitating project management.

## **Activity Logging**

To accurately record activity during work, **the application logs keystrokes, and mouse movements**, and takes screenshots(upcoming releases). This comprehensive data allows for tracking the time spent on specific tasks and identifying potential inefficiencies or distractions. 

This type of logging is crucial for clients because it gives the needed detailed overview of employee productivity and helps to identify areas where improvements can be made. Clients can see exactly what tasks were completed and how much time was spent on each one, as well as monitor productivity levels. 

Important to mention that we are transparent about what data is being collected and how it will be used to avoid any misunderstandings or mistrust.

## **Authentication**

Authentication is a crucial aspect of any project, and we strive to make it as convenient as possible for our users. **By integrating with wallets like MetaMask, we offer a seamless onboarding experience for the Web3 community**, providing freelancers with a convenient authentication method that eliminates the need to create new accounts and remember additional usernames and passwords.

In a login process like this, a freelancer simply scans a QR code containing a message that needs to be signed directly in the app. For that a user opens the wallet application of choice extension and selects the "Scan QR Code" option, then scans the QR code with their device's camera, prompting the wallet to sign the message and send it back to our desktop application.

This authentication process not only ensures quick onboarding but also provides enhanced security and censorship-resistant authentication through the distributed network of Ethereum nodes. It serves as a robust alternative to Single Sign-On, presenting a user-friendly and secure Web3 authentication experience.

## Invoicing and Payments

Automatic time tracking also simplifies the invoicing and payment processes, promoting trust and transparency between freelancers and clients. The tracked time serves as a reliable record for subsequent payments, ensuring that freelancers are accurately compensated for their work.

## **Cross-platform**

Developed with the QT6 framework, Communa TimeTracker is a cross-platform app compatible with the latest versions of Windows, Mac OS, and Linux (Ubuntu, Fedora, Debian). Remote workers can install it with a simple one-click process directly from Communa's GitHub profile, requiring no additional technical knowledge or assistance.
