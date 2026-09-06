/* Run the actual unchanged restore implementation with a showbar=1 checkpoint. */
static void test_restore_hidden_bar(Monitor *m)
{
 ReloadHeader header = {0x44574d52U, 1, 1, 0, 0};
 ReloadMonitor saved = {0};
 FILE *file = tmpfile();
 int fd;
 assert(file);
 saved.showbar = 1; saved.topbar = 1; saved.nmaster = 1;
 saved.tagset[0] = saved.tagset[1] = 1; saved.mfact = 0.55f;
 assert(fwrite(&header, sizeof header, 1, file) == 1);
 assert(fwrite(&saved, sizeof saved, 1, file) == 1);
 assert(!fflush(file)); rewind(file);
 fd = dup(fileno(file)); assert(fd >= 0);
 assert(reload_state_restore(fd));
 assert(!m->showbar && m->wy == 0 && m->wh == 1080);
 assert(mock.compatibility_moves == 1 && mock.maps == 0);
 fclose(file);
}
