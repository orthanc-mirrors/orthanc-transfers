function TransferAcceleratorSelectPeer(callback)
{
  var items = $('<ul>')
    .attr('data-divider-theme', 'd')
    .attr('data-role', 'listview');

  items.append('<li data-role="list-divider">Orthanc peers</li>');

  $.ajax({
    url: '../transfers/peers',
    type: 'GET',
    dataType: 'json',
    async: false,
    cache: false,
    success: function(peers) {
      for (var i = 0; i < peers.length; i++) {
        var name = peers[i];
        var item = $('<li>')
          .html('<a href="#" rel="close">' + name + '</a>')
          .attr('name', name)
          .click(function() { 
            clickedPeer = $(this).attr('name');
          });
        items.append(item);
      }

      // Launch the dialog
      $('#dialog').simpledialog2({
        mode: 'blank',
        animate: false,
        headerText: 'Choose target',
        headerClose: true,
        forceInput: false,
        width: '100%',
        blankContent: items,
        callbackClose: function() {
          var timer;
          function WaitForDialogToClose() {
            if (!$('#dialog').is(':visible')) {
              clearInterval(timer);
              callback(clickedPeer);
            }
          }
          timer = setInterval(WaitForDialogToClose, 100);
        }
      });
    }
  });
}


function TransferAcceleratorAddSendButton(level, siblingButton)
{
  var b = $('<a>')
    .attr('data-role', 'button')
    .attr('href', '#')
    .attr('data-icon', 'search')
    .attr('data-theme', 'e')
    .text('Transfer accelerator');

  b.insertBefore($(siblingButton).parent().parent());

  b.click(function() {
    if ($.mobile.pageData) {
      var uuid = $.mobile.pageData.uuid;
      TransferAcceleratorSelectPeer(function(peer) {
        console.log('Sending ' + level + ' ' + uuid + ' to peer ' + peer);

        var query = {
          'Resources' : [
            {
              'Level' : level,
              'ID' : uuid
            }
          ], 
          'Compression' : 'gzip',
          'Peer' : peer
        };

        $.ajax({
          url: '../transfers/send',
          type: 'POST',
          dataType: 'json',
          data: JSON.stringify(query),
          success: function(job) {
            if (!(typeof job.ID === 'undefined')) {
              $.mobile.changePage('#job?uuid=' + job.ID);
            }
          },
          error: function() {
            alert('Error while creating the transfer job');
          }
        });  
      });
    }
  });
}



$('#patient').live('pagebeforecreate', function() {
  TransferAcceleratorAddSendButton('Patient', '#patient-delete');
});

$('#study').live('pagebeforecreate', function() {
  TransferAcceleratorAddSendButton('Study', '#study-delete');
});

$('#series').live('pagebeforecreate', function() {
  TransferAcceleratorAddSendButton('Series', '#series-delete');
});
