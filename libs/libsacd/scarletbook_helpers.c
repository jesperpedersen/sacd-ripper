/**
 * SACD Ripper - https://github.com/sacd-ripper/
 *
 * Copyright (c) 2010-2015 by respective authors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fileutils.h>
#include <utils.h>

#include "scarletbook_helpers.h"

/* Safe helpers: always NUL-terminate */
static void safe_copy(char *dst, size_t cap, const char *src) {
    if (!dst || cap == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    size_t n = strlen(src);
    if (n >= cap) n = cap - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static void safe_copy_n(char *dst, size_t cap, const char *src, size_t nbytes) {
    if (!dst || cap == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    size_t n = nbytes;
    if (n >= cap) n = cap - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static void safe_append(char *dst, size_t cap, const char *suffix) {
    if (!dst || !suffix || cap == 0) return;
    size_t len = strlen(dst);
    if (len >= cap) return;
    size_t rem = cap - len;
    size_t n = strlen(suffix);
    if (n >= rem) n = rem - 1;
    memcpy(dst + len, suffix, n);
    dst[len + n] = '\0';
}

//  Copy UTF-8 encoding chars
//   *dst  - destination string
//   *src   - source string
//    n - number of chars
//
int utf8cpy(char *dst, char *src, int n)
{
    // n is the size of dst (including the last byte for null
    int i = 0;

    while (i < n)
    {
        int c;
        if (!(src[i] & 0x80))
        {
            // ASCII code
            if (src[i] == '\0')
            {
                break;
            }
            c = 1;
        }
        else if ((src[i] & 0xe0) == 0xc0)
        {
            // 2-byte code
            c = 2;
        }
        else if ((src[i] & 0xf0) == 0xe0)
        {
            // 3-byte code
            c = 3;
        }
        else if ((src[i] & 0xf8) == 0xf0)
        {
            // 4-byte code
            c = 4;
        }
        else if (src[i] == '\0')
        {
            break;
        }
        else
        {
            break;
        }
        
        if (i + c <= n)
        {
            memcpy(dst + i, src + i, (size_t)c * sizeof(char));
            i += c;
        }
        else
        {
            break;
        }
    }

    dst[i] = '\0';
    return i;
}

#define MAX_DISC_ARTIST_LEN 40
#define MAX_ALBUM_TITLE_LEN 100
#define MAX_TRACK_TITLE_LEN 120
#define MAX_TRACK_ARTIST_LEN 40

//  Generates a name for a directory or filename from disc/album artist and album/disc title
//  Useful for iso, cue, xml, DFF edit master files
//    if album is multiset then adds
//         ' (disc number-number_of_total discs)'
//    if artist_flag >0 then adds disc/album artist to the name of directory
//       'artist - title'
//      or
//       'artist - title (disc number-number_of_total discs)'
//   NOTE: caller must free the returned string!
//
char *get_album_dir(scarletbook_handle_t *handle, int artist_flag)
{
    char disc_artist[MAX_DISC_ARTIST_LEN + 1];
    char disc_album_title[MAX_ALBUM_TITLE_LEN + 1];
    char disc_album_year[20];
    char *albumdir = NULL;
    master_text_t *master_text = &handle->master_text;
    char *p_artist = NULL;
    char *p_album_title = NULL;

    if (master_text->disc_artist)
        p_artist = master_text->disc_artist;
    else if (master_text->disc_artist_phonetic)
        p_artist = master_text->disc_artist_phonetic;
    else if (master_text->album_artist)
        p_artist = master_text->album_artist;
    else if (master_text->album_artist_phonetic)
        p_artist = master_text->album_artist_phonetic;


    if (handle->master_toc->album_set_size > 1) // If there is a set of discs
    {
        // in case of multiset, album_title  must be used first, instead of disc_title
        if (master_text->album_title)
            p_album_title = master_text->album_title;
        else if (master_text->album_title_phonetic)
            p_album_title = master_text->album_title_phonetic;
        else if (master_text->disc_title)
            p_album_title = master_text->disc_title;
        else if (master_text->disc_title)
            p_album_title = master_text->disc_title_phonetic;
    }
    else
    {
        if (master_text->disc_title)
            p_album_title = master_text->disc_title;
        else if (master_text->disc_title_phonetic)
            p_album_title = master_text->disc_title_phonetic;
        else if (master_text->album_title)
            p_album_title = master_text->album_title;
        else if (master_text->album_title_phonetic)
            p_album_title = master_text->album_title_phonetic;
    }


    memset(disc_artist, 0, sizeof(disc_artist));
    if (p_artist)
    {
        char *pos = NULL, *pos1 = NULL;

        pos = strchr(p_artist, ';');
        if (!pos)
            pos = p_artist + strlen(p_artist);

        pos1 = strchr(p_artist, '/'); // standard artist separator
        if (pos1 != NULL && pos1 < pos)
            pos = pos1;
        pos1 = strchr(p_artist, ',');
        if (pos1 != NULL && pos1 < pos)
            pos = pos1;
        // pos1 = strchr(p_artist, '.'); 
        // if (pos1 != NULL && pos1 < pos)
        //     pos = pos1;
        pos1 = strstr(p_artist, " -");
        if (pos1 != NULL && pos1 < pos)
            pos = pos1;

        size_t to_copy = (size_t)(pos - p_artist);
        if (to_copy > MAX_DISC_ARTIST_LEN) to_copy = MAX_DISC_ARTIST_LEN;
        safe_copy_n(disc_artist, sizeof(disc_artist), p_artist, to_copy);
        
        sanitize_filename(disc_artist);
    }

    memset(disc_album_title, 0, sizeof(disc_album_title));
    if (p_album_title)
    {
        char *pos = strchr(p_album_title, ';');
        if (!pos)
            pos = p_album_title + strlen(p_album_title);

        size_t to_copy = (size_t)(pos - p_album_title);
        if (to_copy > MAX_ALBUM_TITLE_LEN) to_copy = MAX_ALBUM_TITLE_LEN;
        safe_copy_n(disc_album_title, sizeof(disc_album_title), p_album_title, to_copy);
        
        sanitize_filename(disc_album_title);
    }

    char multiset_s[20] = "";
    char *disc_album_title_final;

    if (handle->master_toc->album_set_size > 1) // Set of discs
    {       
        snprintf(multiset_s, sizeof(multiset_s), " (disc %d-%d)", handle->master_toc->album_sequence_number, handle->master_toc->album_set_size);
        disc_album_title_final = (char *)malloc(strlen(disc_album_title) + strlen(multiset_s) + 1);
        if (!disc_album_title_final) return NULL;
        snprintf(disc_album_title_final, strlen(disc_album_title) + strlen(multiset_s) + 1, "%s%s", disc_album_title, multiset_s);
    }
    else
    {
        disc_album_title_final = (char *)malloc(strlen(disc_album_title) + 1);
        if (!disc_album_title_final) return NULL;
        snprintf(disc_album_title_final, strlen(disc_album_title) + 1, "%s", disc_album_title);
    }


    snprintf(disc_album_year, sizeof(disc_album_year), "%04u", handle->master_toc->disc_date_year);

   
    //sanitize_filename(disc_album_title_final);

    if (strlen(disc_artist) > 0 && strlen(disc_album_title) > 0 && artist_flag !=0 )
        albumdir = parse_format("%A - %L", 0, disc_album_year, disc_artist, disc_album_title_final, NULL);
    else if (strlen(disc_artist) > 0 && artist_flag != 0)
        albumdir = parse_format("%A", 0, disc_album_year, disc_artist, disc_album_title_final, NULL);
    else if (strlen(disc_album_title) > 0)
        albumdir = parse_format("%L", 0, disc_album_year, disc_artist, disc_album_title_final, NULL);
    else
        albumdir = parse_format("Unknown Album", 0, disc_album_year, disc_artist, disc_album_title_final, NULL);

    //sanitize_filepath(albumdir);

    free(disc_album_title_final);
    
    return albumdir;
}

//  Generates a path from disc title/album title.
//  Useful for dsf, dff files.
//  the only difference from get_album_dir is:
//    if album has multiple discs then
//       inserts a new component into path \Disc 1...N
// if artist_flag=1 then adds artist to the path like this:
//    artist - title
//    artist - title\Disc 1...N
//  NOTE: caller must free the returned string!
char *get_path_disc_album(scarletbook_handle_t *handle, int artist_flag)
{
    //char disc_title[MAX_ALBUM_TITLE_LEN + 1];
    char disc_album_title[MAX_ALBUM_TITLE_LEN + 1];
    char disc_artist[MAX_DISC_ARTIST_LEN + 1];

    master_text_t *master_text = &handle->master_text;
     
    //char *p_disc_title = NULL;
    char *p_album_title = NULL;
    char *p_artist = NULL;
    char *disc_album_title_final=NULL;
    

    if (handle->master_toc->album_set_size > 1) // If there is a set of discs
    {
        // in case of multiset, album_title  must be used first, instead of disc_title
        if (master_text->album_title)
            p_album_title = master_text->album_title;
        else if (master_text->album_title_phonetic)
            p_album_title = master_text->album_title_phonetic;
        else if (master_text->disc_title)
            p_album_title = master_text->disc_title;
        else if (master_text->disc_title)
            p_album_title = master_text->disc_title_phonetic;
    }
    else
    {
        if (master_text->disc_title)
            p_album_title = master_text->disc_title;
        else if (master_text->disc_title_phonetic)
            p_album_title = master_text->disc_title_phonetic;
        else if (master_text->album_title)
            p_album_title = master_text->album_title;
        else if (master_text->album_title_phonetic)
            p_album_title = master_text->album_title_phonetic;
    }

    if (master_text->disc_artist)
        p_artist = master_text->disc_artist;
    else if (master_text->disc_artist_phonetic)
        p_artist = master_text->disc_artist_phonetic;
    else if (master_text->album_artist)
        p_artist = master_text->album_artist;
    else if (master_text->album_artist_phonetic)
        p_artist = master_text->album_artist_phonetic;


    memset(disc_album_title, 0,sizeof(disc_album_title));
    if (p_album_title)
    {
        size_t to_copy = strlen(p_album_title);
        if (to_copy > MAX_ALBUM_TITLE_LEN) to_copy = MAX_ALBUM_TITLE_LEN;
        safe_copy_n(disc_album_title, sizeof(disc_album_title), p_album_title, to_copy);
        sanitize_filename(disc_album_title);
    }
    else
    {
        safe_copy(disc_album_title, sizeof(disc_album_title), "unknown album title");
    }

    memset(disc_artist, 0, sizeof(disc_artist));
    if (p_artist)
    {
        char *pos=NULL, *pos1=NULL;
        
        pos = strchr(p_artist, ';');
        if (!pos)
            pos = p_artist + strlen(p_artist);
        
        pos1 = strchr(p_artist, '/'); // standard artist separator
        if (pos1 != NULL && pos1 < pos)
            pos = pos1;
        pos1 = strchr(p_artist, ',');
        if (pos1 != NULL && pos1 < pos)
            pos = pos1;
        // pos1 = strchr(p_artist, '.'); 
        // if (pos1 != NULL && pos1 < pos)
        //     pos = pos1;
        pos1 = strstr(p_artist, " -");
        if (pos1 != NULL && pos1 < pos)
            pos = pos1;


        size_t to_copy = (size_t)(pos - p_artist);
        if (to_copy > MAX_DISC_ARTIST_LEN) to_copy = MAX_DISC_ARTIST_LEN;
        safe_copy_n(disc_artist, sizeof(disc_artist), p_artist, to_copy);
        sanitize_filename(disc_artist);
    }

    if (handle->master_toc->album_set_size > 1) // If there is a set of discs
    {
        char multiset_s[40];

        memset(multiset_s, 0, sizeof(multiset_s));
        //snprintf(multiset_s, sizeof(multiset_s), "(disc %d of %d)", handle->master_toc->album_sequence_number, handle->master_toc->album_set_size);
        snprintf(multiset_s, sizeof(multiset_s), "Disc %d", handle->master_toc->album_sequence_number);

        if (artist_flag !=0  && strlen(disc_artist) > 0) // add artist name
        {
            size_t need = strlen(disc_artist) + 3 + strlen(disc_album_title) + 1 + strlen(multiset_s) + 1;
            disc_album_title_final = (char *)calloc(1, need);
            if (!disc_album_title_final) return NULL;
            snprintf(disc_album_title_final, need, "%s - %s", disc_artist, disc_album_title);
        }
        else 
        {
            size_t need = strlen(disc_album_title) + 1 + strlen(multiset_s) + 1;
            disc_album_title_final = (char *)calloc(1, need);
            if (!disc_album_title_final) return NULL;
            snprintf(disc_album_title_final, need, "%s", disc_album_title);
        }

#if defined(WIN32) || defined(_WIN32)
        safe_append(disc_album_title_final, strlen(disc_album_title_final) + 1 + strlen(multiset_s) + 1, "\\");
#else
        safe_append(disc_album_title_final, strlen(disc_album_title_final) + 1 + strlen(multiset_s) + 1, "/");
#endif
        safe_append(disc_album_title_final, strlen(disc_album_title_final) + 1 + strlen(multiset_s) + 1, multiset_s);
    }
    else   // not album set
    {
        if (artist_flag !=0 && strlen(disc_artist) > 0) // add artist name
        {
            size_t need = strlen(disc_artist) + 3 + strlen(disc_album_title) + 1;
            disc_album_title_final = (char *)calloc(1, need);
            if (!disc_album_title_final) return NULL;
            snprintf(disc_album_title_final, need, "%s - %s", disc_artist, disc_album_title);
        }
        else{
            size_t need = strlen(disc_album_title) + 1;
            disc_album_title_final = (char *)calloc(1, need);
            if (!disc_album_title_final) return NULL;
            snprintf(disc_album_title_final, need, "%s", disc_album_title);
        }
    }

    //sanitize_filepath(disc_album_title_final);
    
    return disc_album_title_final;
}

char *get_music_filename(scarletbook_handle_t *handle, int area, int track, const char *override_title, int performer_flag)
{
    char *c =NULL;
    char track_artist[MAX_TRACK_ARTIST_LEN + 1];
    char track_title[MAX_TRACK_TITLE_LEN + 1];
    char disc_album_title[MAX_ALBUM_TITLE_LEN + 1];
    char disc_album_year[20];
    master_text_t *master_text = &handle->master_text;
    char * p_album_title = NULL;
    int performer_flag_local=performer_flag;

    memset(track_artist, 0, sizeof(track_artist));
    if( handle->area[area].area_track_text[track].track_type_performer)
    {
        c = handle->area[area].area_track_text[track].track_type_performer;
    }
    else
    {
        if (master_text->disc_artist)
        {
            c = master_text->disc_artist;           
        }
        else if (master_text->album_artist)
        {
            c =  master_text->album_artist;           
        }
        else
        {
            // nothing found to put in performer. Do not insert it all
            //strncpy(track_artist, "unknown performer", min(strlen("unknown performer"), MAX_TRACK_ARTIST_LEN));
            performer_flag_local = 0;
        }       
        
    }
    

    if (c)
    {
        char *pos = NULL, *pos1 = NULL;

        pos = strchr(c, ';');
        if (!pos)
            pos = c + strlen(c);

        pos1 = strchr(c, '/'); // standard artist separator
        if (pos1 != NULL && pos1 < pos)
            pos = pos1;
        pos1 = strchr(c, ',');
        if (pos1 != NULL && pos1 < pos)
            pos = pos1;
        // pos1 = strchr(c, '.'); 
        // if (pos1 != NULL && pos1 < pos)
        //     pos = pos1;
        pos1 = strstr(c, " -");
        if (pos1 != NULL && pos1 < pos)
            pos = pos1;
		
        size_t to_copy = (size_t)(pos - c);
        if (to_copy > MAX_TRACK_ARTIST_LEN) to_copy = MAX_TRACK_ARTIST_LEN;
        safe_copy_n(track_artist, sizeof(track_artist), c, to_copy);
        sanitize_filename(track_artist);
    }
    

    memset(track_title, 0, sizeof(track_title));
    c = handle->area[area].area_track_text[track].track_type_title;
    if (c)
    {
        safe_copy(track_title, sizeof(track_title), c);
        sanitize_filename(track_title);
    }
    else
    {
        safe_copy(track_title, sizeof(track_title), "unknown track title");
    }
    
    if (master_text->disc_title)
        p_album_title = master_text->disc_title;
    else if (master_text->disc_title_phonetic)
        p_album_title = master_text->disc_title_phonetic;
    else if (master_text->album_title)
        p_album_title = master_text->album_title;
    else if (master_text->album_title_phonetic)
        p_album_title = master_text->album_title_phonetic;

    memset(disc_album_title, 0, sizeof(disc_album_title));
    if (p_album_title)
    {
        char *pos = strchr(p_album_title, ';');
        if (!pos)
            pos = p_album_title + strlen(p_album_title);
        size_t to_copy = (size_t)(pos - p_album_title);
        if (to_copy > MAX_ALBUM_TITLE_LEN) to_copy = MAX_ALBUM_TITLE_LEN;
        safe_copy_n(disc_album_title, sizeof(disc_album_title), p_album_title, to_copy);
        sanitize_filename(disc_album_title);
    }
    else
    {
        safe_copy(disc_album_title, sizeof(disc_album_title), "unknown title");
    }
    
    snprintf(disc_album_year, sizeof(disc_album_year), "%04u", handle->master_toc->disc_date_year);

    if (override_title && strlen(override_title) > 0)
        {
            if(performer_flag_local ==0)
                return parse_format("%N - %L -%T", track + 1, disc_album_year, track_artist, disc_album_title, override_title);
            else
                return parse_format("%N - %L - %A -%T", track + 1, disc_album_year, track_artist, disc_album_title, override_title);
        }
    else if (strlen(track_artist) > 0 && strlen(track_title) > 0 && performer_flag_local != 0) 
        return parse_format("%N - %A - %T", track + 1, disc_album_year, track_artist, disc_album_title, track_title);
    else if (strlen(track_artist) > 0 && performer_flag_local != 0)  
        return parse_format("%N - %A", track + 1, disc_album_year, track_artist, disc_album_title, track_title);
    else if (strlen(track_title) > 0)
        return parse_format("%N - %T", track + 1, disc_album_year, track_artist, disc_album_title, track_title);
    else if (strlen(disc_album_title) > 0)
        return parse_format("%N - %L", track + 1, disc_album_year, track_artist, disc_album_title, track_title);
    else
        return parse_format("%N - Unknown Artist", track + 1, disc_album_year, track_artist, disc_album_title, track_title);
}


char *get_speaker_config_string(area_toc_t *area) 
{
    if (area->channel_count == 2 && area->extra_settings == 0)
    {
        return "Stereo";
    }
    else if (area->channel_count == 5 && area->extra_settings == 3)
    {
        return "5ch";
    }
    else if (area->channel_count == 6 && area->extra_settings == 4)
    {
        return "6ch"; // "5.1ch";
    }
    else
    {
        return "Unkn-ch";
    }
}

char *get_frame_format_string(area_toc_t *area) 
{
    if (area->frame_format == FRAME_FORMAT_DSD_3_IN_14)
    {
        return "DSD 3 in 14";
    }
    else if (area->frame_format == FRAME_FORMAT_DSD_3_IN_16)
    {
        return "DSD 3 in 16";
    }
    else if (area->frame_format == FRAME_FORMAT_DST)
    {
        return "Lossless DST";
    }
    else
    {
        return "Unknown";
    }
}