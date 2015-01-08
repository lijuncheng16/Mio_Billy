<!DOCTYPE html>
<html lang="en-US">
  <head>
    <meta charset="utf-8">
  </head>
  <body>
    <h2>Mio Password Reset</h2>
    <div>
      Hello <?php echo $user['name'];?>
      <p>To reset your password, follow this link or copy this code: <?php echo link_to('/#/recovery_password/'.$user['username'].'/'.$token, $token); ?></p>
      <p>If you did not request this email, please ignore it</p>
    </div>
  </body>
</html>
